// script.js — block-based LC-3 FSM simulator front-end.
//
// The user composes programs from drag-droppable opcode blocks (Scratch-like).
// Each block has fillable fields (registers, immediates, labels). Pressing
// "Run" walks the block list, encodes each block into a 16-bit machine word
// (resolving label references to PC-offsets), and loads the result into the
// WASM CPU. From the WASM boundary onward, behavior is identical to the
// previous build — your unchanged C++ FSM runs the program.

// ============================================================================
// 1. BLOCK CATALOG
// ----------------------------------------------------------------------------
// Each entry describes one opcode-shaped block: its visual category (for
// coloring), the fields it shows, and a function that encodes a block of this
// type into a uint16_t machine word given its current field values + the
// symbol table.
// ============================================================================

const REGS = ['R0','R1','R2','R3','R4','R5','R6','R7'];
const sext = (v, bits) => v & ((1 << bits) - 1);
const fits = (v, bits) => v >= -(1<<(bits-1)) && v <= (1<<(bits-1))-1;

// Each field descriptor: { name, kind, options?, default, width? }
//   kind: 'reg' | 'imm' | 'mode' | 'nzp' | 'target' | 'trap'
const CATALOG = {
  ADD: {
    family:'exec', label:'ADD',
    fields:[
      {name:'DR',  kind:'reg', default:'R0'},
      {name:'SR1', kind:'reg', default:'R0'},
      {name:'mode',kind:'mode',options:['SR2','imm'], default:'imm'},
      {name:'op2', kind:'regOrImm', default:'0', width:5},
    ],
    encode(f){
      const dr=+f.DR.slice(1), s1=+f.SR1.slice(1);
      if(f.mode==='SR2'){ const r2=+f.op2.slice(1); return (1<<12)|(dr<<9)|(s1<<6)|r2; }
      const im=parseInt(f.op2,10);
      if(!fits(im,5)) throw new Error(`ADD imm5 out of range: ${im}`);
      return (1<<12)|(dr<<9)|(s1<<6)|(1<<5)|sext(im,5);
    }
  },
  AND: {
    family:'exec', label:'AND',
    fields:[
      {name:'DR',  kind:'reg', default:'R0'},
      {name:'SR1', kind:'reg', default:'R0'},
      {name:'mode',kind:'mode',options:['SR2','imm'], default:'imm'},
      {name:'op2', kind:'regOrImm', default:'0', width:5},
    ],
    encode(f){
      const dr=+f.DR.slice(1), s1=+f.SR1.slice(1);
      if(f.mode==='SR2'){ const r2=+f.op2.slice(1); return (5<<12)|(dr<<9)|(s1<<6)|r2; }
      const im=parseInt(f.op2,10);
      if(!fits(im,5)) throw new Error(`AND imm5 out of range: ${im}`);
      return (5<<12)|(dr<<9)|(s1<<6)|(1<<5)|sext(im,5);
    }
  },
  NOT: {
    family:'exec', label:'NOT',
    fields:[{name:'DR',kind:'reg',default:'R0'},{name:'SR',kind:'reg',default:'R0'}],
    encode(f){ const dr=+f.DR.slice(1), sr=+f.SR.slice(1); return (9<<12)|(dr<<9)|(sr<<6)|0x3F; }
  },
  LD: {
    family:'mem', label:'LD',
    fields:[{name:'DR',kind:'reg',default:'R0'},{name:'target',kind:'target',width:9}],
    encode(f,ctx){ const dr=+f.DR.slice(1); return (2<<12)|(dr<<9)|resolveOffset(f.target,9,ctx); }
  },
  LDR: {
    family:'mem', label:'LDR',
    fields:[
      {name:'DR',kind:'reg',default:'R0'},
      {name:'BaseR',kind:'reg',default:'R0'},
      {name:'off6',kind:'imm',default:'0',width:6},
    ],
    encode(f){
      const dr=+f.DR.slice(1), b=+f.BaseR.slice(1), im=parseInt(f.off6,10);
      if(!fits(im,6)) throw new Error(`LDR offset6 out of range: ${im}`);
      return (6<<12)|(dr<<9)|(b<<6)|sext(im,6);
    }
  },
  LDI: {
    family:'mem', label:'LDI',
    fields:[{name:'DR',kind:'reg',default:'R0'},{name:'target',kind:'target',width:9}],
    encode(f,ctx){ const dr=+f.DR.slice(1); return (10<<12)|(dr<<9)|resolveOffset(f.target,9,ctx); }
  },
  ST: {
    family:'mem', label:'ST',
    fields:[{name:'SR',kind:'reg',default:'R0'},{name:'target',kind:'target',width:9}],
    encode(f,ctx){ const sr=+f.SR.slice(1); return (3<<12)|(sr<<9)|resolveOffset(f.target,9,ctx); }
  },
  STR: {
    family:'mem', label:'STR',
    fields:[
      {name:'SR',kind:'reg',default:'R0'},
      {name:'BaseR',kind:'reg',default:'R0'},
      {name:'off6',kind:'imm',default:'0',width:6},
    ],
    encode(f){
      const sr=+f.SR.slice(1), b=+f.BaseR.slice(1), im=parseInt(f.off6,10);
      if(!fits(im,6)) throw new Error(`STR offset6 out of range: ${im}`);
      return (7<<12)|(sr<<9)|(b<<6)|sext(im,6);
    }
  },
  STI: {
    family:'mem', label:'STI',
    fields:[{name:'SR',kind:'reg',default:'R0'},{name:'target',kind:'target',width:9}],
    encode(f,ctx){ const sr=+f.SR.slice(1); return (11<<12)|(sr<<9)|resolveOffset(f.target,9,ctx); }
  },
  LEA: {
    family:'exec', label:'LEA',
    fields:[{name:'DR',kind:'reg',default:'R0'},{name:'target',kind:'target',width:9}],
    encode(f,ctx){ const dr=+f.DR.slice(1); return (14<<12)|(dr<<9)|resolveOffset(f.target,9,ctx); }
  },
  BR: {
    family:'ctrl', label:'BR',
    fields:[
      {name:'nzp',kind:'nzp',default:'nzp'},
      {name:'target',kind:'target',width:9},
    ],
    encode(f,ctx){
      const su=f.nzp.toLowerCase();
      let n=su.includes('n'), z=su.includes('z'), p=su.includes('p');
      if(!n&&!z&&!p){n=z=p=true;}
      return (0<<12)|(n?1:0)<<11|(z?1:0)<<10|(p?1:0)<<9|resolveOffset(f.target,9,ctx);
    }
  },
  JMP: {
    family:'ctrl', label:'JMP',
    fields:[{name:'BaseR',kind:'reg',default:'R7'}],
    encode(f){ const b=+f.BaseR.slice(1); return (12<<12)|(b<<6); }
  },
  JSR: {
    family:'ctrl', label:'JSR',
    fields:[{name:'target',kind:'target',width:11}],
    encode(f,ctx){ return (4<<12)|(1<<11)|resolveOffset(f.target,11,ctx); }
  },
  JSRR: {
    family:'ctrl', label:'JSRR',
    fields:[{name:'BaseR',kind:'reg',default:'R0'}],
    encode(f){ const b=+f.BaseR.slice(1); return (4<<12)|(b<<6); }
  },
  TRAP: {
    family:'sys', label:'TRAP',
    fields:[{name:'vector',kind:'trap',options:['x20','x21','x22','x23','x24','x25'],default:'x25'}],
    encode(f){ const v=parseInt(f.vector.replace(/^x/,''),16); return (15<<12)|(v&0xFF); }
  },
  HALT: {
    family:'sys', label:'HALT',
    fields:[],
    encode(){ return (15<<12)|0x25; }
  },
};

// ============================================================================
// 2. PROGRAM MODEL
// ----------------------------------------------------------------------------
// The program is an array of block instances. Each instance:
//   { id, opcode, fields:{...}, label:string|null }
// 'label' (if set) is the name of any other block can branch to.
// ============================================================================

let program = [];           // ordered list of block instances
let nextId = 1;
function makeBlock(opcode){
  const spec = CATALOG[opcode];
  const fields = {};
  for(const f of spec.fields) fields[f.name] = f.default ?? '';
  return { id:'b'+(nextId++), opcode, fields, label:null };
}

// ============================================================================
// 3. ENCODER (blocks -> uint16_t array)
// ----------------------------------------------------------------------------
// Two passes, just like the textual assembler — pass 1 maps each block's label
// (if any) to its address; pass 2 encodes each block, resolving target dropdowns
// against the symbol table.
// ============================================================================

const ORIG = 0x3000;

function resolveOffset(target, bits, ctx){
  // target is either a label name (string) or a numeric offset (e.g. "-2")
  let dest;
  if (target === '' || target === '(none)') throw new Error('branch/load target is empty');
  if (target.startsWith('#') || /^-?\d+$/.test(target)) {
    // numeric offset, used directly as the PC-offset value
    const v = parseInt(target.replace('#',''), 10);
    if(!fits(v, bits)) throw new Error(`target offset ${v} out of range (${bits}-bit signed)`);
    return sext(v, bits);
  }
  // label
  if (!(target in ctx.symbols)) throw new Error(`unknown label "${target}"`);
  dest = ctx.symbols[target];
  const off = dest - (ctx.addr + 1);
  if(!fits(off, bits)) throw new Error(`label "${target}" is too far (${off}, ${bits}-bit signed range)`);
  return sext(off, bits);
}

function encode(){
  // Pass 1: addresses + symbol table
  const symbols = {};
  program.forEach((b,i)=>{
    if (b.label){
      if (symbols[b.label] !== undefined) throw new Error(`duplicate label "${b.label}"`);
      symbols[b.label] = ORIG + i;
    }
  });
  // Pass 2: encode each block in order
  const words = program.map((b,i)=>{
    const spec = CATALOG[b.opcode];
    try { return spec.encode(b.fields, { addr: ORIG+i, symbols }) & 0xFFFF; }
    catch(e){ throw new Error(`block ${i+1} (${b.opcode}): ${e.message}`); }
  });
  return { words, orig: ORIG, symbols };
}

// Collect labels that exist in the program (for target dropdowns)
function availableLabels(){ return program.filter(b=>b.label).map(b=>b.label); }

// ============================================================================
// 4. UI: palette, program list, drag/drop, field editors
// ============================================================================

const $ = s => document.querySelector(s);
const hex = v => '0x'+(v>>>0).toString(16).toUpperCase().padStart(4,'0');

function renderPalette(){
  const pal = $('#palette');
  pal.innerHTML = '';
  for (const op of Object.keys(CATALOG)){
    const spec = CATALOG[op];
    const el = document.createElement('div');
    el.className = `palette-block fam-${spec.family}`;
    el.textContent = spec.label;
    el.draggable = true;
    el.dataset.opcode = op;
    el.addEventListener('dragstart', e=>{
      e.dataTransfer.setData('application/x-lc3-op', op);
      e.dataTransfer.effectAllowed = 'copy';
    });
    el.addEventListener('click', ()=>{ program.push(makeBlock(op)); renderProgram(); });
    pal.appendChild(el);
  }
}

function renderProgram(){
  const list = $('#program');
  list.innerHTML = '';
  if (program.length === 0){
    list.innerHTML = '<div class="empty">drag blocks here, or click them in the palette →</div>';
    return;
  }
  const labels = availableLabels();
  program.forEach((b, idx)=>{
    const spec = CATALOG[b.opcode];
    const el = document.createElement('div');
    el.className = `block fam-${spec.family}`;
    el.dataset.id = b.id;
    el.draggable = true;
    el.innerHTML = `
      <div class="block-head">
        <span class="addr">${hex(ORIG+idx)}</span>
        <span class="opcode">${spec.label}</span>
        <span class="labeltag">${b.label
          ? `<span class="lbl">${b.label}<button class="lbldel" title="remove label">×</button></span>`
          : `<button class="lbladd">+ label</button>`}</span>
        <button class="del" title="delete block">×</button>
      </div>
      <div class="fields"></div>`;
    const fieldsEl = el.querySelector('.fields');
    for (const f of spec.fields){
      fieldsEl.appendChild(renderField(b, f, labels));
    }
    // Label button
    el.querySelector('.lbladd')?.addEventListener('click', ()=>{
      const name = prompt('Label name (e.g. LOOP):', '');
      if (!name) return;
      if (program.some(x=>x.label===name)) { alert('Label already used'); return; }
      b.label = name.trim();
      renderProgram();
    });
    el.querySelector('.lbldel')?.addEventListener('click', ()=>{
      b.label = null; renderProgram();
    });
    el.querySelector('.del').addEventListener('click', ()=>{
      program = program.filter(x=>x.id!==b.id); renderProgram();
    });
    // Drag-reorder
    el.addEventListener('dragstart', e=>{
      e.dataTransfer.setData('application/x-lc3-move', b.id);
      e.dataTransfer.effectAllowed = 'move';
    });
    el.addEventListener('dragover', e=>{ e.preventDefault(); el.classList.add('drop-above'); });
    el.addEventListener('dragleave', ()=>el.classList.remove('drop-above'));
    el.addEventListener('drop', e=>{
      e.preventDefault(); el.classList.remove('drop-above');
      handleDrop(e, idx);
    });
    list.appendChild(el);
  });
}

function renderField(block, fieldSpec, labels){
  const wrap = document.createElement('label');
  wrap.className = 'field';
  wrap.innerHTML = `<span class="fname">${fieldSpec.name}</span>`;
  const val = block.fields[fieldSpec.name];

  if (fieldSpec.kind === 'reg'){
    const sel = document.createElement('select');
    for (const r of REGS){ const o=document.createElement('option'); o.value=r; o.textContent=r; if(r===val) o.selected=true; sel.appendChild(o); }
    sel.onchange = ()=>{ block.fields[fieldSpec.name]=sel.value; };
    wrap.appendChild(sel);
  }
  else if (fieldSpec.kind === 'mode'){
    const sel = document.createElement('select');
    for (const opt of fieldSpec.options){ const o=document.createElement('option'); o.value=opt; o.textContent=opt; if(opt===val) o.selected=true; sel.appendChild(o); }
    sel.onchange = ()=>{
      block.fields[fieldSpec.name]=sel.value;
      // Toggle op2 default: switching to SR2 should set a register-shaped value
      if (sel.value==='SR2' && !/^R[0-7]$/.test(block.fields.op2)) block.fields.op2 = 'R0';
      if (sel.value==='imm' && /^R[0-7]$/.test(block.fields.op2)) block.fields.op2 = '0';
      renderProgram();
    };
    wrap.appendChild(sel);
  }
  else if (fieldSpec.kind === 'regOrImm'){
    if (block.fields.mode === 'SR2'){
      const sel = document.createElement('select');
      for (const r of REGS){ const o=document.createElement('option'); o.value=r; o.textContent=r; if(r===val) o.selected=true; sel.appendChild(o); }
      sel.onchange = ()=>{ block.fields[fieldSpec.name]=sel.value; };
      wrap.appendChild(sel);
    } else {
      const inp = document.createElement('input');
      inp.type='number'; inp.value=val; inp.className='imm';
      inp.min = -(1<<(fieldSpec.width-1)); inp.max = (1<<(fieldSpec.width-1))-1;
      inp.onchange = ()=>{ block.fields[fieldSpec.name]=inp.value; };
      wrap.appendChild(inp);
    }
  }
  else if (fieldSpec.kind === 'imm'){
    const inp = document.createElement('input');
    inp.type='number'; inp.value=val; inp.className='imm';
    inp.min = -(1<<(fieldSpec.width-1)); inp.max = (1<<(fieldSpec.width-1))-1;
    inp.onchange = ()=>{ block.fields[fieldSpec.name]=inp.value; };
    wrap.appendChild(inp);
  }
  else if (fieldSpec.kind === 'nzp'){
    const span = document.createElement('span'); span.className='nzpfield';
    for (const c of ['n','z','p']){
      const id='nzp'+block.id+c;
      const cb=document.createElement('input'); cb.type='checkbox'; cb.id=id;
      cb.checked = val.includes(c);
      const lbl=document.createElement('label'); lbl.htmlFor=id; lbl.textContent=c.toUpperCase();
      cb.onchange = ()=>{
        const cur = (cb.checked? c : '') + (span.querySelector('input[id$="z"]').checked?'z':'') + (span.querySelector('input[id$="p"]').checked?'p':'');
        // rebuild from current state
        const n=span.querySelector('input[id$="n"]').checked?'n':'';
        const z=span.querySelector('input[id$="z"]').checked?'z':'';
        const p=span.querySelector('input[id$="p"]').checked?'p':'';
        block.fields[fieldSpec.name] = (n+z+p)||'nzp';
      };
      span.appendChild(cb); span.appendChild(lbl);
    }
    wrap.appendChild(span);
  }
  else if (fieldSpec.kind === 'target'){
    const sel = document.createElement('select'); sel.className='target';
    const empty=document.createElement('option'); empty.value=''; empty.textContent='(choose target)'; sel.appendChild(empty);
    for (const l of labels){
      const o=document.createElement('option'); o.value=l; o.textContent='↳ '+l;
      if (l===val) o.selected=true; sel.appendChild(o);
    }
    const numOpt=document.createElement('option'); numOpt.value='__num__'; numOpt.textContent='— numeric offset —';
    sel.appendChild(numOpt);
    if (val && !labels.includes(val) && val!=='') sel.value = '__num__';
    sel.onchange = ()=>{
      if (sel.value==='__num__'){
        const n = prompt(`PC-offset (signed ${fieldSpec.width}-bit, e.g. -2 or 3):`, '0');
        if(n===null) { renderProgram(); return; }
        block.fields[fieldSpec.name] = n;
      } else {
        block.fields[fieldSpec.name] = sel.value;
      }
      renderProgram();
    };
    // Show current numeric value if not a label
    if (val && !labels.includes(val) && val!==''){
      const cur = document.createElement('span'); cur.className='curnum'; cur.textContent=' ='+val;
      wrap.appendChild(cur);
    }
    wrap.appendChild(sel);
  }
  else if (fieldSpec.kind === 'trap'){
    const sel = document.createElement('select');
    for (const opt of fieldSpec.options){
      const o=document.createElement('option'); o.value=opt; o.textContent=opt+(opt==='x25'?' (HALT)':opt==='x21'?' (OUT)':opt==='x22'?' (PUTS)':opt==='x20'?' (GETC)':'');
      if(opt===val) o.selected=true; sel.appendChild(o);
    }
    sel.onchange = ()=>{ block.fields[fieldSpec.name]=sel.value; };
    wrap.appendChild(sel);
  }
  return wrap;
}

function handleDrop(e, targetIdx){
  const opcode = e.dataTransfer.getData('application/x-lc3-op');
  const moveId = e.dataTransfer.getData('application/x-lc3-move');
  if (opcode){
    program.splice(targetIdx, 0, makeBlock(opcode));
  } else if (moveId){
    const fromIdx = program.findIndex(b=>b.id===moveId);
    if (fromIdx<0 || fromIdx===targetIdx) return;
    const [moved] = program.splice(fromIdx,1);
    const insertAt = fromIdx < targetIdx ? targetIdx-1 : targetIdx;
    program.splice(insertAt, 0, moved);
  }
  renderProgram();
}

// Drop onto the empty program area or after the last block
function wireProgramDrop(){
  const list = $('#program');
  list.addEventListener('dragover', e=>{ e.preventDefault(); });
  list.addEventListener('drop', e=>{
    if (e.target.closest('.block')) return; // handled by block drop
    e.preventDefault();
    handleDrop(e, program.length);
  });
}

// ============================================================================
// 5. WASM driver (unchanged from before — loads cpu, steps, renders trace)
// ============================================================================

const STATE_NUM = {
  FETCH_0:18, FETCH_1:33, FETCH_2:35, DECODE:32,
  ADD_0:1, AND_0:5, NOT_0:9, LEA_0:14,
  LD_0:2, LD_1:25, LD_2:27, LDR_0:6, LDR_1:25, LDR_2:27,
  LDI_0:10, LDI_1:24, LDI_2:26, LDI_3:25, LDI_4:27,
  ST_0:3, ST_1:23, ST_2:16, STR_0:7, STR_1:23, STR_2:16,
  STI_0:11, STI_1:29, STI_2:31, STI_3:23, STI_4:16,
  JSR_0:4, "JSR, IR11=0":20, "JSR, IR[11]=1":21, JMP_0:12,
  BR_0:0, BR_1:22, TRAP_0:15, TRAP_1:28, TRAP_2:30,
};
function family(name, desc){
  if (name.startsWith('FETCH')) return 'fetch';
  if (name === 'DECODE') return 'decode';
  if (name === 'HALT' || name === 'ERROR' || name.includes('RESERVED')) return 'halt';
  if (/M\[MAR\]/.test(desc)) return 'mem';
  return 'exec';
}

let Module=null, cpu=null, trace=[], curIdx=-1, timer=null;

async function boot(){
  renderPalette(); wireProgramDrop(); renderProgram();
  wireControls();
  const engine = $('#engine');
  try {
    const createLC3 = (await import('./lc3.js')).default;
    Module = await createLC3();
    cpu = new Module.LC3();
    // Success: leave the engine pill hidden so the header stays clean.
    engine.hidden = true;
    engine.classList.remove('error');
    render(true);
  } catch(e){
    // Failure: replace the cycle pill area with a visible red warning.
    engine.hidden = false;
    engine.classList.add('error');
    engine.textContent = '⚠ WASM failed to load';
    $('#err').textContent =
      'Could not load lc3.wasm. Run `make wasm`, then serve over http (not file://).\n('+e.message+')';
  }
}

function loadAndReset(){
  stopRun();
  if (!cpu){ $('#err').textContent='WASM not loaded yet'; return; }
  if (program.length===0){ $('#err').textContent='Add some blocks first.'; return; }
  let prog;
  try { prog = encode(); } catch(e){ $('#err').textContent = e.message; return; }
  $('#err').textContent='';
  cpu.reset();
  cpu.loadProgram(prog.words, prog.orig);
  trace=[]; curIdx=-1;
  render(true);
}

function stepCycle(){
  if (!cpu || !cpu.running()) return;
  const name=cpu.stateName(), desc=cpu.stateDesc();
  trace.push({ name, desc, snum:STATE_NUM[name]??null, fam:family(name,desc) });
  curIdx = trace.length-1;
  cpu.step();
  render();
  if (!cpu.running()) stopRun();
}
function stepInstruction(){
  if (!cpu || !cpu.running()) return;
  let g=0; do { stepCycle(); g++; } while(cpu.running() && cpu.stateName()!=='FETCH_0' && g<80);
}
function startRun(){
  if (timer || !cpu) return;
  $('#run').disabled=true; $('#pause').disabled=false;
  const tick=()=>{ if(!cpu.running()){stopRun();return;} stepCycle(); timer=setTimeout(tick, +$('#speed').value); };
  tick();
}
function stopRun(){ if(timer){clearTimeout(timer);timer=null;} $('#run').disabled=false; $('#pause').disabled=true; }

function render(initial){
  if (!cpu) return;
  if (curIdx>=0){
    const t=trace[curIdx];
    $('#stagenow').innerHTML = `now in state <b>${t.name}</b>`;
    $('#rtltext').textContent = t.desc;
    $('#snum').textContent = t.snum===null?'—':'#'+t.snum;
  } else {
    $('#stagenow').textContent = cpu.halted()?'halted':(initial?'ready':'stopped');
    $('#rtltext').textContent = initial?'ready':'';
    $('#snum').textContent='';
  }
  $('#cyclepill').textContent = cpu.cycles()+' cycles';

  let rh='';
  for(let i=0;i<8;i++){
    const v=cpu.reg(i);
    rh+=`<tr><td class="name">R${i}</td><td class="val">${hex(v)}</td><td class="name">${v>0x7FFF?v-0x10000:v}</td></tr>`;
  }
  rh+=`<tr><td class="name">PC</td><td class="val">${hex(cpu.pc())}</td><td></td></tr>`;
  rh+=`<tr><td class="name">IR</td><td class="val">${hex(cpu.ir())}</td><td></td></tr>`;
  rh+=`<tr><td class="name">MAR</td><td class="val">${hex(cpu.mar())}</td><td></td></tr>`;
  rh+=`<tr><td class="name">MDR</td><td class="val">${hex(cpu.mdr())}</td><td></td></tr>`;
  $('#regs').innerHTML=rh;

  document.querySelectorAll('#nzp span').forEach(s=>{
    const f=s.dataset.f, on=f==='n'?cpu.n():f==='z'?cpu.z():cpu.p();
    s.classList.toggle('on',on); s.textContent=f.toUpperCase()+' '+(on?1:0);
  });

  drawDatapath();
  renderTrace();
  highlightActiveBlock();
}

// Highlight which block the CPU is currently fetching/executing
function highlightActiveBlock(){
  document.querySelectorAll('#program .block').forEach(el=>el.classList.remove('active'));
  if (!cpu) return;
  const pc = cpu.pc();
  // The currently-executing instruction is typically PC-1 (PC was already incremented by fetch_0).
  const ir = cpu.ir();
  const idxByIR = ir ? (cpu.pc()-1 - ORIG) : -1;
  const idx = idxByIR >= 0 && idxByIR < program.length ? idxByIR : (pc - ORIG);
  if (idx>=0 && idx<program.length){
    const el = document.querySelector(`.block[data-id="${program[idx].id}"]`);
    if (el) el.classList.add('active');
  }
}

function renderTrace(){
  const box=$('#trace');
  box.innerHTML = trace.map((t,i)=>`
    <div class="row${i===curIdx?' cur':''}">
      <span class="sn">${t.snum===null?'·':t.snum}</span>
      <span class="st tag-${t.fam}">${t.name}</span>
      <span class="ds">${t.desc}</span>
    </div>`).join('') || '<div class="empty">no cycles yet</div>';
  $('#tracecount').textContent = trace.length?trace.length+' cycles':'';
  const cur=box.querySelector('.cur'); if(cur && cur.scrollIntoView) cur.scrollIntoView({block:'nearest'});
}

function drawDatapath(){
  if (!cpu) return;
  const t = curIdx>=0?trace[curIdx]:null;
  const col = {fetch:'#3b82f6',decode:'#a855f7',exec:'#10b981',mem:'#f97316',halt:'#ef4444'}[t?t.fam:'fetch'];
  const desc = t?t.desc:'';
  const name = t?t.name:'';
  const lit = {
    PC:  name==='FETCH_0' || /PC <-/.test(desc),
    MAR: /MAR/.test(desc),
    MDR: /MDR/.test(desc),
    IR:  name==='FETCH_2',
    MEM: /M\[MAR\]/.test(desc),
    ALU: /ADD|AND|NOT/.test(name),
  };
  const node=(x,y,w,label,val,on)=>`
    <g><rect x="${x}" y="${y}" width="${w}" height="40" rx="8"
      fill="${on?col:'#1c232d'}" stroke="${on?col:'#2a3340'}" stroke-width="1.5"/>
      <text x="${x+12}" y="${y+17}" fill="${on?'#04120c':'#8b98a8'}" font-family="monospace" font-size="11">${label}</text>
      <text x="${x+12}" y="${y+32}" fill="${on?'#04120c':'#e6edf3'}" font-family="monospace" font-size="13" font-weight="600">${val}</text></g>`;
  const memVal = hex(cpu.readMem(cpu.mar()));
  $('#datapath').innerHTML = `
  <svg width="100%" viewBox="0 0 540 250" role="img">
    <line x1="120" y1="70" x2="120" y2="118" stroke="#2a3340" stroke-width="1.2"/>
    <line x1="120" y1="158" x2="120" y2="196" stroke="#2a3340" stroke-width="1.2"/>
    <line x1="180" y1="138" x2="220" y2="138" stroke="#2a3340" stroke-width="1.2"/>
    ${node(60,30,120,'PC',hex(cpu.pc()),lit.PC)}
    ${node(360,30,120,'IR',hex(cpu.ir()),lit.IR)}
    ${node(60,118,120,'MAR',hex(cpu.mar()),lit.MAR)}
    ${node(60,196,120,'MDR',hex(cpu.mdr()),lit.MDR)}
    ${node(220,118,120,'MEMORY',memVal,lit.MEM)}
    ${node(360,118,120,'ALU / regs',lit.ALU?'compute':'idle',lit.ALU)}
    <text x="270" y="238" text-anchor="middle" fill="#5b6776" font-family="monospace" font-size="11">${name?name+' — '+desc:'idle'}</text>
  </svg>`;
}

function wireControls(){
  $('#assemble').onclick=loadAndReset;
  $('#stepCycle').onclick=stepCycle;
  $('#stepInstr').onclick=stepInstruction;
  $('#run').onclick=startRun;
  $('#pause').onclick=stopRun;
  $('#reset').onclick=loadAndReset;
  $('#clear').onclick=()=>{ program=[]; renderProgram(); };
  // Preset programs
  $('#preset').onchange = (e)=>{
    const v=e.target.value; if(!v) return;
    loadPreset(v); e.target.value='';
  };
}

const PRESETS = {
  countdown: ()=>[
    Object.assign(makeBlock('AND'),{fields:{DR:'R0',SR1:'R0',mode:'imm',op2:'0'}}),
    Object.assign(makeBlock('ADD'),{fields:{DR:'R0',SR1:'R0',mode:'imm',op2:'5'}}),
    Object.assign(makeBlock('AND'),{fields:{DR:'R1',SR1:'R1',mode:'imm',op2:'0'}}),
    Object.assign(makeBlock('ADD'),{fields:{DR:'R1',SR1:'R1',mode:'SR2',op2:'R0'}, label:'LOOP'}),
    Object.assign(makeBlock('ADD'),{fields:{DR:'R0',SR1:'R0',mode:'imm',op2:'-1'}}),
    Object.assign(makeBlock('BR'), {fields:{nzp:'p',target:'LOOP'}}),
    makeBlock('HALT'),
  ],
  addtwo: ()=>[
    Object.assign(makeBlock('ADD'),{fields:{DR:'R0',SR1:'R0',mode:'imm',op2:'5'}}),
    Object.assign(makeBlock('ADD'),{fields:{DR:'R1',SR1:'R1',mode:'imm',op2:'3'}}),
    Object.assign(makeBlock('ADD'),{fields:{DR:'R2',SR1:'R0',mode:'SR2',op2:'R1'}}),
    makeBlock('HALT'),
  ],
  notreg: ()=>[
    Object.assign(makeBlock('AND'),{fields:{DR:'R0',SR1:'R0',mode:'imm',op2:'0'}}),
    Object.assign(makeBlock('NOT'),{fields:{DR:'R0',SR:'R0'}}),
    makeBlock('HALT'),
  ],
};
function loadPreset(name){
  if (!(name in PRESETS)) return;
  program = PRESETS[name]();
  // Re-key block IDs so they're unique
  program.forEach(b=>{ b.id='b'+(nextId++); });
  renderProgram();
}

boot();