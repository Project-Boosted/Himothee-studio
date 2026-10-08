import streamDeck, {action, SingletonAction, type KeyDownEvent, type WillAppearEvent, type DidReceiveSettingsEvent} from "@elgato/streamdeck";
import {discover, execute, EMPTY_STATE, type ActionDefinition, type Snapshot} from "./bridge";
import {configured, validate, visual, type Kind, type Settings} from "./logic";

let online = false;
let actions: ActionDefinition[] = [];
let state: Snapshot = EMPTY_STATE;
let polling = false;
const visible = new Map<string, {action: any; kind: Kind; settings: Settings; last: string}>();
const pending = new Set<string>();
const lastPress = new Map<string, number>();

function inspectorUpdate() {
 // SDK routes this only to the currently visible inspector.
 void streamDeck.ui.sendToPropertyInspector({type:"snapshot", online, actions, state}).catch(()=>{});
}
async function paint(context: string) {
 const entry = visible.get(context);
 if (!entry || !entry.action.isKey()) return;
 const v = visual(entry.kind, entry.settings, state, online);
 const serialized = JSON.stringify(v);
 if (serialized === entry.last) return;
 entry.last = serialized;
 try {
  await entry.action.setState(v.active ? 1 : 0);
  await entry.action.setTitle(v.title);
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
class BaseControl extends SingletonAction<Settings> {
 constructor(private readonly kind: Kind) {super();}
 override async onWillAppear(ev: WillAppearEvent<Settings>) {
  visible.set(ev.action.id, {action:ev.action,kind:this.kind,settings:ev.payload.settings||{},last:""});
  await paint(ev.action.id);
 }
 override onWillDisappear(ev: any) {
  visible.delete(ev.action.id); pending.delete(ev.action.id); lastPress.delete(ev.action.id);
 }
 override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<Settings>) {
  const item=visible.get(ev.action.id);
  if (item) {item.settings=ev.payload.settings||{};item.last=""; await paint(ev.action.id);}
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
setInterval(()=>{void poll();},1500);
void streamDeck.connect();
