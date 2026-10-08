import type {ActionDefinition, Parameters, Snapshot} from "./bridge";
export type Kind = "command"|"stream"|"record"|"destination"|"scene"|"overlay"|"counter"|"timer";
export type Settings = {
 actionId?: string; params?: Record<string, unknown>;
 destinationId?: string; scene?: string; overlayId?: string; amount?: number; timerMode?: string;
};
export type Command = {id: string; params: Parameters};
export function configured(kind: Kind, settings: Settings, state: Snapshot): Command | null {
 const overlayId = settings.overlayId?.trim();
 const destinationId = settings.destinationId?.trim();
 switch (kind) {
  case "stream": return {id: state.streaming ? "stream.stop" : "stream.start", params:{}};
  case "record": return {id: state.recording ? "record.stop" : "record.start", params:{}};
  case "destination": {
   if (!destinationId) return null;
   const dest = state.destinations.find(x=>x.id===destinationId);
   if (!dest) return null;
   const active = ["active", "starting", "reconnecting", "prepared"].includes(dest.state);
   return {id: active ? "destination.stop" : "destination.start", params:{destination_id:destinationId}};
  }
  case "scene": return settings.scene?.trim() ? {id:"scene.switch",params:{scene:settings.scene.trim()}} : null;
  case "overlay": {
   const item = state.overlays.find(x=>x.id===overlayId);
   return item ? {id:item.visible ? "overlay.hide":"overlay.show",params:{overlay_id:item.id}} : null;
  }
  case "counter": {
   if (!state.overlays.some(x=>x.id===overlayId)) return null;
   const amount = Number(settings.amount ?? 1);
   if (!Number.isInteger(amount) || amount < 1 || amount > 100) return null;
   return {id:"counter.increment",params:{overlay_id:overlayId!,amount}};
  }
  case "timer": {
   if (!state.overlays.some(x=>x.id===overlayId)) return null;
   const mode = settings.timerMode || "timer.start";
   return ["timer.start","timer.pause","timer.reset"].includes(mode)
     ? {id:mode,params:{overlay_id:overlayId!}} : null;
  }
  case "command": {
   const id = settings.actionId || "";
   if (!id) return null;
   const raw = settings.params || {};
   const params: Parameters = {};
   for (const [key,value] of Object.entries(raw)) {
    if (typeof value === "string" || typeof value === "number" || typeof value === "boolean") {
     params[key] = value;
    }
   }
   return {id,params};
  }
 }
}
export function validate(command: Command | null, actions: ActionDefinition[]): boolean {
 if (!command) return false;
 const definition = actions.find(a => a.id === command.id);
 if (!definition) return false;
 for (const [name, spec] of Object.entries(definition.parameters || {})) {
   const v = command.params[name];
   if (spec.required && (v === undefined || v === "" || v === null)) return false;
   if (v !== undefined && v !== "") {
     if (spec.type === "number" && (typeof v !== "number" || !Number.isFinite(v))) return false;
     if (spec.type === "string" && typeof v !== "string") return false;
   }
 }
 return true;
}
export function visual(kind: Kind, settings: Settings, state: Snapshot, online: boolean) {
 if (!online) return {active:false,title:"STUDIO\nOFFLINE"};
 const overlay = state.overlays.find(x=>x.id===settings.overlayId);
 const dest = state.destinations.find(x=>x.id===settings.destinationId);
 switch (kind) {
  case "stream": return {active:state.streaming,title:state.streaming ? "STREAM\nLIVE":"START\nSTREAM"};
  case "record": return {active:state.recording,title:state.recording ? "RECORD\nON":"START\nRECORD"};
  case "destination": return {active:!!dest && ["active","starting","reconnecting"].includes(dest.state),
   title:dest ? `${dest.name.slice(0,12)}\n${dest.state.toUpperCase()}` : "SELECT\nOUTPUT"};
  case "scene": return {active:!!settings.scene && settings.scene===state.scene,
   title:settings.scene ? `SCENE\n${settings.scene.slice(0,14)}` : "SELECT\nSCENE"};
  case "overlay": return {active:!!overlay?.visible,
   title:overlay ? `${overlay.name.slice(0,13)}\n${overlay.visible?"SHOWN":"HIDDEN"}` : "SELECT\nOVERLAY"};
  case "counter": return {active:!!overlay,
   title:overlay ? `${overlay.name.slice(0,13)}\n${String(overlay.display??"0").slice(0,12)}` : "SELECT\nCOUNTER"};
  case "timer": return {active:!!overlay,
   title:overlay ? `${overlay.name.slice(0,13)}\n${String(overlay.display??"").slice(0,12)}` : "SELECT\nTIMER"};
  case "command": return {active:!!settings.actionId,title:settings.actionId ? settings.actionId.replace(".", "\n").slice(0,23) : "SELECT\nACTION"};
  case "dial-counter": case "dial-scene": case "dial-timer":
   return {active:false,title:"DIAL"};
 }
}
