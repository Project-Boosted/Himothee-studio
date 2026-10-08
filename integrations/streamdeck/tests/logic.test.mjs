import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import {transformSync} from "esbuild";
// Execute the pure command planner without connecting to Stream Deck or OBS.
const src = fs.readFileSync(new URL("../src/logic.ts", import.meta.url),"utf8");
const js = transformSync(src,{loader:"ts",format:"cjs",target:"node24"}).code;
const moduleObj={exports:{}};
new Function("module","exports",js)(moduleObj,moduleObj.exports);
const {configured,validate,visual}=moduleObj.exports;
const state={ready:true,streaming:false,recording:true,replay_buffer:false,scene:"Game",
 scenes:["Game","BRB"],destinations:[{id:"kick",name:"Kick",state:"idle"}],
 overlays:[{id:"c1",name:"180 Counter",type:"counter",visible:true,display:12},
 {id:"timer",name:"Timer",type:"timer",visible:false,display:"01:00"}]};
const defs=[
 {id:"stream.start",parameters:{}},{id:"destination.start",parameters:{destination_id:{type:"string",required:true}}},
 {id:"counter.increment",parameters:{overlay_id:{type:"string",required:true},amount:{type:"number",required:false}}}
];
test("stream chooses explicit start/stop",()=>{assert.equal(configured("stream",{},state).id,"stream.start");assert.equal(configured("stream",{}, {...state,streaming:true}).id,"stream.stop");});
test("destination only starts selected configured output",()=>{assert.deepEqual(configured("destination",{destinationId:"kick"},state),{id:"destination.start",params:{destination_id:"kick"}});assert.equal(configured("destination",{destinationId:"missing"},state),null);});
test("counter validates amount and uses overlay id",()=>{assert.deepEqual(configured("counter",{overlayId:"c1",amount:2},state).params,{overlay_id:"c1",amount:2});assert.equal(configured("counter",{overlayId:"c1",amount:0},state),null);});
test("schema validation prevents incomplete actions",()=>{assert.equal(validate(configured("destination",{destinationId:"kick"},state),defs),true);assert.equal(validate({id:"destination.start",params:{}},defs),false);});
test("offline and live output feedback",()=>{assert.match(visual("stream",{},state,false).title,/OFFLINE/);assert.equal(visual("record",{},state,true).active,true);assert.match(visual("counter",{overlayId:"c1"},state,true).title,/12/);});
