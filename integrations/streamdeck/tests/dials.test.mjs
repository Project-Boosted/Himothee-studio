import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import {transformSync} from "esbuild";
function pure(file) {
 const ts=fs.readFileSync(new URL(file,import.meta.url),"utf8");
 const compiled=transformSync(ts,{loader:"ts",format:"cjs",target:"node24"}).code;
 const obj={exports:{}};
 new Function("module","exports",compiled)(obj,obj.exports);
 return obj.exports;
}
const {counterRotation,counterPress,timerDial,chooseScene,scenePress,dialVisual}=pure("../src/dials.ts");
const state={
 ready:true,streaming:true,recording:false,replay_buffer:false,scene:"Game",
 scenes:["Game","BRB","Starting"],
 destinations:[],
 overlays:[
  {id:"c",name:"180 Counter",type:"darts180",counter:true,timer:false,visible:true,display:"17",running:false},
  {id:"t",name:"Timer",type:"stopwatch",counter:false,timer:true,visible:true,display:"00:13",running:true}
 ]
};
test("rotation keeps the tick sign and sends bounded incremental actions",()=>{
 assert.deepEqual(counterRotation({overlayId:"c",dialStep:5},state,2),[
  {id:"counter.increment",params:{overlay_id:"c",amount:10}}
 ]);
 assert.deepEqual(counterRotation({overlayId:"c",dialStep:2},state,-3),[
  {id:"counter.decrement",params:{overlay_id:"c",amount:6}}
 ]);
});
test("fast rotations split into requests capped at 100",()=>{
 assert.deepEqual(counterRotation({overlayId:"c",dialStep:10},state,24).map(x=>x.params.amount),[100,100,40]);
});
test("invalid ticks, mismatched types, oversized steps are rejected",()=>{
 assert.deepEqual(counterRotation({overlayId:"c",dialStep:11},state,1),[]);
 assert.deepEqual(counterRotation({overlayId:"c"},state,25),[]);
 assert.deepEqual(counterRotation({overlayId:"t"},state,1),[]);
 assert.deepEqual(counterRotation({overlayId:"no"},state,1),[]);
});
test("dial press increments; long touch requires explicit reset opt-in",()=>{
 assert.deepEqual(counterPress({overlayId:"c",dialStep:5},state),{id:"counter.increment",params:{overlay_id:"c",amount:5}});
 assert.equal(counterPress({overlayId:"c"},state,true),null);
 assert.deepEqual(counterPress({overlayId:"c",allowReset:true},state,true),{id:"counter.reset",params:{overlay_id:"c"}});
});
test("scene selection wraps and previews without live action",()=>{
 assert.equal(chooseScene("Starting",1,state.scenes,state.scene),"Game");
 assert.equal(chooseScene("Game",-1,state.scenes,state.scene),"Starting");
 assert.equal(chooseScene("Game",24,state.scenes,state.scene),"Game");
 assert.equal(chooseScene("Game",25,state.scenes,state.scene),null);
 assert.deepEqual(scenePress("BRB",state),{id:"scene.switch",params:{scene:"BRB"}});
 assert.equal(scenePress("Missing",state),null);
 assert.match(dialVisual("dial-scene",{},state,true,"BRB").title,/PREVIEW/);
});
test("timer press uses actual running state, not an inferred toggle",()=>{
 assert.deepEqual(timerDial({overlayId:"t"},state,"toggle"),{id:"timer.pause",params:{overlay_id:"t"}});
 const paused={...state,overlays:state.overlays.map(x=>x.id==="t"?{...x,running:false}:x)};
 assert.deepEqual(timerDial({overlayId:"t"},paused,"toggle"),{id:"timer.start",params:{overlay_id:"t"}});
 const old={...state,overlays:state.overlays.map(x=>x.id==="t"?{...x,running:undefined}:x)};
 assert.equal(timerDial({overlayId:"t"},old,"toggle"),null);
});
test("timer resets never happen accidentally",()=>{
 assert.equal(timerDial({overlayId:"t"},state,"reset"),null);
 assert.deepEqual(timerDial({overlayId:"t",allowReset:true},state,"reset"),{id:"timer.reset",params:{overlay_id:"t"}});
 assert.equal(timerDial({overlayId:"c"},state,"start"),null);
});
test("touch strip gives offline feedback and readable live values",()=>{
 assert.equal(dialVisual("dial-counter",{overlayId:"c"},state,false).value,"—");
 assert.match(dialVisual("dial-counter",{overlayId:"c",dialStep:2},state,true).value,/17/);
 assert.match(dialVisual("dial-timer",{overlayId:"t"},state,true).title,/RUN/);
});
