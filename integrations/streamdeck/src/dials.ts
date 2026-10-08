import type {Command, Settings} from "./logic";
import type {Snapshot, Overlay} from "./bridge";
export type DialKind = "dial-counter" | "dial-scene" | "dial-timer";

function chosenOverlay(settings: Settings, state: Snapshot): Overlay | undefined {
 return state.overlays.find(x => x.id === settings.overlayId);
}
function capable(overlay: Overlay | undefined, property: "counter" | "timer"): boolean {
 // Older bridges do not report capabilities; the registry validates those requests.
 return !!overlay && overlay[property] !== false;
}
export function counterRotation(settings: Settings, state: Snapshot, ticks: number): Command[] {
 const overlay = chosenOverlay(settings,state);
 const step = Number(settings.dialStep ?? 1);
 if (!capable(overlay,"counter") || !Number.isInteger(step) || step < 1 || step > 10
    || !Number.isInteger(ticks) || ticks === 0 || Math.abs(ticks) > 24) return [];
 const id = ticks > 0 ? "counter.increment" : "counter.decrement";
 let remaining = Math.abs(ticks) * step;
 const commands: Command[] = [];
 while(remaining > 0) {
  const amount = Math.min(100,remaining);
  commands.push({id,params:{overlay_id:overlay!.id,amount}});
  remaining -= amount;
 }
 return commands;
}
export function counterPress(settings: Settings, state: Snapshot, hold = false): Command | null {
 const overlay = chosenOverlay(settings,state);
 if (!capable(overlay,"counter")) return null;
 if (hold) return settings.allowReset === true ? {id:"counter.reset",params:{overlay_id:overlay!.id}} : null;
 const step = Number(settings.dialStep ?? 1);
 if (!Number.isInteger(step)|| step < 1 || step > 10) return null;
 return {id:"counter.increment",params:{overlay_id:overlay!.id,amount:step}};
}
export function timerDial(settings: Settings, state: Snapshot, operation: "toggle" | "start" | "pause" | "reset"): Command | null {
 const overlay = chosenOverlay(settings,state);
 if (!capable(overlay,"timer")) return null;
 if (operation === "reset" && settings.allowReset !== true) return null;
 // Do not guess the timer state when connected to a pre-10.6.4 bridge.
 if (operation === "toggle" && typeof overlay!.running !== "boolean") return null;
 const command = operation === "toggle" ? (overlay!.running ? "timer.pause" : "timer.start") : `timer.${operation}`;
 return {id:command,params:{overlay_id:overlay!.id}};
}
export function chooseScene(current: string | undefined, ticks: number, scenes: string[], liveScene: string): string | null {
 if (!Number.isInteger(ticks) || ticks === 0 || Math.abs(ticks) > 24 || !scenes.length) return null;
 const anchor = scenes.indexOf(current || liveScene);
 const start = anchor < 0 ? Math.max(0,scenes.indexOf(liveScene)) : anchor;
 return scenes[((start + ticks) % scenes.length + scenes.length) % scenes.length];
}
export function scenePress(selected: string | undefined, state: Snapshot): Command | null {
 return selected && state.scenes?.includes(selected)
  ? {id:"scene.switch",params:{scene:selected}} : null;
}
export function dialVisual(kind: DialKind, settings: Settings, state: Snapshot, online: boolean, selection?: string) {
 if (!online) return {title:"HIMOTHEE OFFLINE",value:"—"};
 if (kind === "dial-scene") {
  const chosen = selection || state.scene;
  return {title: chosen === state.scene ? "SCENE • LIVE" : "SCENE • PREVIEW",
   value: chosen?.slice(0,22) || "Choose scene"};
 }
 const item = chosenOverlay(settings,state);
 if (!item) return {title:kind === "dial-counter" ? "COUNTER" : "TIMER",value:"Choose overlay"};
 const value = String(item.display ?? "—");
 return kind === "dial-counter"
  ? {title:item.name.slice(0,22),value:`${value.slice(0,12)} (±${settings.dialStep || 1})`}
  : {title:`${item.name.slice(0,18)} • ${item.running===true?"RUN":item.running===false?"PAUSE":"?"}`,value:value.slice(0,22)};
}
