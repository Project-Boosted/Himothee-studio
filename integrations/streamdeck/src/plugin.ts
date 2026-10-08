import streamDeck, {action, SingletonAction, type KeyDownEvent, type WillAppearEvent, type DidReceiveSettingsEvent, type DialRotateEvent, type DialDownEvent, type TouchTapEvent} from "@elgato/streamdeck";
import {discover, execute, EMPTY_STATE, type ActionDefinition, type Snapshot} from "./bridge";
import {configured, validate, visual, type Kind, type Settings, type Command} from "./logic";
import {counterRotation, counterPress, timerDial, chooseScene, scenePress, dialVisual} from "./dials";

let online = false;
let actions: ActionDefinition[] = [];
let state: Snapshot = EMPTY_STATE;
let polling = false;
type Visible = {action: any; kind: Kind; settings: Settings; last: string; sceneSelection?: string};
const visible = new Map<string, Visible>();
type Queue = {commands: Command[]; busy: boolean};
const dialQueues = new Map<string, Queue>();
const pending = new Set<string>();
const lastPress = new Map<string, number>();

function inspectorUpdate() {
 // SDK routes this only to the currently visible inspector.
 void streamDeck.ui.sendToPropertyInspector({type:"snapshot", online, actions, state}).catch(()=>{});
}
async function paint(context: string) {
 const entry = visible.get(context);
 if (!entry || (!entry.action.isKey() && !entry.action.isDial())) return;
 if (entry.kind === "dial-scene" && state.scenes?.length &&
     entry.sceneSelection && !state.scenes.includes(entry.sceneSelection)) {
   entry.sceneSelection = state.scene;
 }
 const v = entry.action.isDial()
  ? dialVisual(entry.kind as "dial-counter"|"dial-scene"|"dial-timer",
      entry.settings, state, online, entry.sceneSelection)
  : visual(entry.kind, entry.settings, state, online);
 const serialized = JSON.stringify(v);
 if (serialized === entry.last) return;
 entry.last = serialized;
 try {
  if (entry.action.isDial()) {
    await entry.action.setFeedback({title:v.title, value:(v as {value:string}).value});
  } else {
    await entry.action.setState((v as {active:boolean}).active ? 1 : 0);
    await entry.action.setTitle(v.title);
  }
 } catch (err) {
  streamDeck.logger.warn(`Cannot update key: ${String(err)}`);
 }
}
function repaint() {
 for (const context of visible.keys()) void paint(context);
 inspectorUpdate();
}
async function poll() {
 if (polling) return;
 polling = true;
 try {
  const result = await discover();
  online = true; state = result.state; actions = result.actions;
 } catch {
  online = false; state = EMPTY_STATE; actions = [];
 } finally {
  polling = false;
  repaint();
 }
}
/** No automatic replay: retrying a timed-out command could double-adjust a counter. */
async function issue(context: string, action: any, command: Command | null): Promise<void> {
 if (!online || !validate(command, actions)) {await action.showAlert(); return;}
 if (pending.has(context) || dialQueues.get(context)?.busy) return;
 pending.add(context);
 try {
   const result = await execute(command!.id, command!.params);
   if (!result.success) throw new Error(`${result.code}: ${result.message}`);
   await poll();
 } catch (err) {
   streamDeck.logger.warn(`Himothee action error: ${String(err)}`);
   await action.showAlert();
 } finally {pending.delete(context);}
}
/** Preserve dial tick ordering even while a previous HTTP command is in flight. */
function enqueue(context: string, action: any, commands: Command[]) {
 if (!commands.length) return;
 const queue = dialQueues.get(context) || {commands:[],busy:false};
 if (queue.commands.length + commands.length > 24) {
  streamDeck.logger.warn("Dial queue full; rejecting rotation rather than silently dropping it.");
  void action.showAlert();return;
 }
 queue.commands.push(...commands);
 dialQueues.set(context,queue);
 if (!queue.busy) void drain(context,queue,action);
}
async function drain(context: string, queue: Queue, action: any) {
 queue.busy = true;
 let changed = false;
 try {
  while(queue.commands.length && dialQueues.get(context)===queue && visible.has(context)) {
   if (!online) throw new Error("Studio disconnected while processing dial commands");
   const command = queue.commands.shift()!;
   if (!validate(command,actions)) throw new Error("Dial command is no longer registered");
   const result = await execute(command.id,command.params);
   if (!result.success) throw new Error(`${result.code}: ${result.message}`);
   changed = true;
  }
 } catch(err) {
  queue.commands.length = 0;
  streamDeck.logger.warn(`Dial adjustment failed (no automatic retry): ${String(err)}`);
  await action.showAlert();
 } finally {
  queue.busy = false;
  if (dialQueues.get(context)===queue) dialQueues.delete(context);
  if (changed) await poll();
 }
}
class BaseControl extends SingletonAction<Settings> {
 constructor(private readonly kind: Kind) {super();}
 override async onWillAppear(ev: WillAppearEvent<Settings>) {
  visible.set(ev.action.id, {action:ev.action,kind:this.kind,settings:ev.payload.settings||{},last:"",
    sceneSelection:this.kind==="dial-scene" ? state.scene : undefined});
  if (ev.action.isDial()) await ev.action.setFeedbackLayout("$A1");
  await paint(ev.action.id);
 }
 override onWillDisappear(ev: any) {
  visible.delete(ev.action.id); pending.delete(ev.action.id); lastPress.delete(ev.action.id);
  dialQueues.delete(ev.action.id);
 }
 override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<Settings>) {
  const item=visible.get(ev.action.id);
  if (item) {item.settings=ev.payload.settings||{};item.last="";
    if (this.kind==="dial-scene") item.sceneSelection=state.scene;
    await paint(ev.action.id);}
 }
 override async onKeyDown(ev: KeyDownEvent<Settings>) {
  const context=ev.action.id;
  // Guard duplicate concurrent presses and keyboard bounce; no retries after an ambiguous request.
  if (pending.has(context) || Date.now()-(lastPress.get(context)||0)<350) return;
  lastPress.set(context,Date.now());
  if (!online) {await ev.action.showAlert();return;}
  const command=configured(this.kind, ev.payload.settings||{}, state);
  if (!validate(command, actions)) {await ev.action.showAlert();return;}
  pending.add(context);
  try {
   const result=await execute(command!.id,command!.params);
   if (!result.success) {
    streamDeck.logger.warn(`Himothee action ${command!.id}: ${result.code}: ${result.message}`);
    await ev.action.showAlert();
   } else {await poll();}
  } catch(err) {
   streamDeck.logger.warn(`Himothee request failed: ${String(err)}`);
   await ev.action.showAlert();
  } finally {pending.delete(context);}
 }
 override async onDialRotate(ev: DialRotateEvent<Settings>) {
  if (!online || ev.payload.pressed) return;
  const context = ev.action.id;
  const entry = visible.get(context);
  if (!entry) return;
  if (this.kind==="dial-counter") {
   enqueue(context,ev.action,counterRotation(entry.settings,state,ev.payload.ticks));
  } else if (this.kind==="dial-scene") {
   const selected=chooseScene(entry.sceneSelection,ev.payload.ticks,state.scenes||[],state.scene);
   if (selected) {entry.sceneSelection=selected;entry.last="";await paint(context);}
  } else if (this.kind==="dial-timer") {
   const op=ev.payload.ticks>0?"start":"pause";
   if (ev.payload.ticks!==0) await issue(context,ev.action,timerDial(entry.settings,state,op));
  }
 }
 override async onDialDown(ev: DialDownEvent<Settings>) {
  if (!online) {await ev.action.showAlert();return;}
  const context=ev.action.id, entry=visible.get(context);
  if (!entry) return;
  if (this.kind==="dial-counter") {
   const command=counterPress(entry.settings,state);
   if (!validate(command,actions)) {await ev.action.showAlert();return;}
   enqueue(context,ev.action,[command!]);
  } else if (this.kind==="dial-scene") {
   await issue(context,ev.action,scenePress(entry.sceneSelection||state.scene,state));
  } else if (this.kind==="dial-timer") {
   await issue(context,ev.action,timerDial(entry.settings,state,"toggle"));
  }
 }
 override async onTouchTap(ev: TouchTapEvent<Settings>) {
  if (!online) {await ev.action.showAlert();return;}
  const context=ev.action.id, entry=visible.get(context);
  if (!entry) return;
  if (this.kind==="dial-counter") {
   const command=counterPress(entry.settings,state,ev.payload.hold);
   if (command && validate(command,actions)) enqueue(context,ev.action,[command]);
  } else if (this.kind==="dial-timer") {
   if (ev.payload.hold && !entry.settings.allowReset) return;
   await issue(context,ev.action,timerDial(entry.settings,state,ev.payload.hold?"reset":"toggle"));
  } else if (this.kind==="dial-scene" && !ev.payload.hold) {
   await issue(context,ev.action,scenePress(entry.sceneSelection||state.scene,state));
  }
 }
 override async onPropertyInspectorDidAppear() {
  await poll();
  inspectorUpdate();
 }
 override async onSendToPlugin(ev: any) {
  if (ev.payload?.type === "refresh") {await poll(); inspectorUpdate();}
 }
}
@action({UUID:"com.himothee.studio.command"})
class CommandAction extends BaseControl {constructor(){super("command");}}
@action({UUID:"com.himothee.studio.stream"})
class StreamAction extends BaseControl {constructor(){super("stream");}}
@action({UUID:"com.himothee.studio.record"})
class RecordAction extends BaseControl {constructor(){super("record");}}
@action({UUID:"com.himothee.studio.destination"})
class DestinationAction extends BaseControl {constructor(){super("destination");}}
@action({UUID:"com.himothee.studio.scene"})
class SceneAction extends BaseControl {constructor(){super("scene");}}
@action({UUID:"com.himothee.studio.overlay"})
class OverlayAction extends BaseControl {constructor(){super("overlay");}}
@action({UUID:"com.himothee.studio.counter"})
class CounterAction extends BaseControl {constructor(){super("counter");}}
@action({UUID:"com.himothee.studio.timer"})
class TimerAction extends BaseControl {constructor(){super("timer");}}

streamDeck.actions.registerAction(new CommandAction());
streamDeck.actions.registerAction(new StreamAction());
streamDeck.actions.registerAction(new RecordAction());
streamDeck.actions.registerAction(new DestinationAction());
streamDeck.actions.registerAction(new SceneAction());
streamDeck.actions.registerAction(new OverlayAction());
streamDeck.actions.registerAction(new CounterAction());
streamDeck.actions.registerAction(new TimerAction());
@action({UUID:"com.himothee.studio.dial-counter"})
class DialCounterAction extends BaseControl {constructor(){super("dial-counter");}}
@action({UUID:"com.himothee.studio.dial-scene"})
class DialSceneAction extends BaseControl {constructor(){super("dial-scene");}}
@action({UUID:"com.himothee.studio.dial-timer"})
class DialTimerAction extends BaseControl {constructor(){super("dial-timer");}}
streamDeck.actions.registerAction(new DialCounterAction());
streamDeck.actions.registerAction(new DialSceneAction());
streamDeck.actions.registerAction(new DialTimerAction());
setInterval(()=>{void poll();},1500);
void streamDeck.connect();
