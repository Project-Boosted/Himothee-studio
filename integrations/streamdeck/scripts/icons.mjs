import fs from "node:fs";
import path from "node:path";
import {deflateSync} from "node:zlib";
const out = path.resolve("com.himothee.studio.sdPlugin/imgs");
fs.mkdirSync(out,{recursive:true});
const crcTable = new Uint32Array(256);
for (let i=0;i<256;i++){let c=i;for(let k=0;k<8;k++)c=c&1?0xEDB88320^(c>>>1):c>>>1;crcTable[i]=c>>>0;}
function crc(buf){let c=0xffffffff;for(const b of buf)c=crcTable[(c^b)&255]^(c>>>8);return (c^0xffffffff)>>>0;}
function chunk(type,data){const tag=Buffer.from(type);const len=Buffer.alloc(4);len.writeUInt32BE(data.length);const checksum=Buffer.alloc(4);checksum.writeUInt32BE(crc(Buffer.concat([tag,data])));return Buffer.concat([len,tag,data,checksum]);}
function png(n,mode) {
 const raw=Buffer.alloc(n*(1+n*4));
 const px=(x,y,r,g,b,a=255)=>{const i=y*(1+n*4)+1+x*4;raw[i]=r;raw[i+1]=g;raw[i+2]=b;raw[i+3]=a;};
 for(let y=0;y<n;y++)for(let x=0;x<n;x++){
  const X=(x+.5)/n,Y=(y+.5)/n;
  const mono=mode==="category"||mode==="action";
  const corners=Math.max(Math.abs(X-.5),Math.abs(Y-.5));
  const inside = corners <.47 && !(corners>.40 && Math.hypot(X<.5?X-.40:X-.60,Y<.5?Y-.40:Y-.60)>.102);
  if (!mono && inside) {
   const active=mode==="key-on";
   const bg=active?[15,86,61]:mode==="key-off"?[32,40,57]:[19,35,58];
   px(x,y,...bg);
  }
  const h=((X>.23&&X<.36)||(X>.64&&X<.77))&&Y>.22&&Y<.79 ||
        (X>.32&&X<.68&&Y>.45&&Y<.57);
  if(h)px(x,y,255,255,255,255);
 }
 const head=Buffer.alloc(13);head.writeUInt32BE(n,0);head.writeUInt32BE(n,4);head[8]=8;head[9]=6;
 return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk("IHDR",head),chunk("IDAT",deflateSync(raw)),chunk("IEND",Buffer.alloc(0))]);
}
for(const [name,size] of Object.entries({"plugin":256,"category":28,"action":20,"key-off":72,"key-on":72})){
 for (const [suffix,scale] of [["",1],["@2x",2]]){
  fs.writeFileSync(path.join(out,name+suffix+".png"),png(size*scale,name));
 }
}
