/** Strictly local v1 bridge client. Never accept an endpoint URL from action settings. */
export const BASE_URL = "http://127.0.0.1:3294";
export type Parameters = Record<string, string | number | boolean>;
export type ActionDefinition = {
 id: string; name: string; category: string; description: string;
 parameters: Record<string, {type: string; required: boolean; description: string}>;
};
export type Overlay = {id: string; name: string; type: string; visible: boolean; display: unknown};
export type Destination = {id: string; name: string; state: string};
export type Snapshot = {
 ready: boolean; streaming: boolean; recording: boolean; replay_buffer: boolean;
 scene: string; scenes?: string[]; destinations: Destination[]; overlays: Overlay[];
};
export type ExecuteResult = {success: boolean; code: string; message: string};
export const EMPTY_STATE: Snapshot = {
 ready: false, streaming: false, recording: false, replay_buffer: false, scene: "",
 scenes: [], destinations: [], overlays: []
};
async function readJson(path: string, init?: RequestInit): Promise<any> {
 const response = await fetch(BASE_URL + path, {
   ...init, cache: "no-store", signal: AbortSignal.timeout(1600)
 });
 if (!response.ok) throw new Error(`Himothee bridge: HTTP ${response.status}`);
 return response.json();
}
export async function discover(): Promise<{actions: ActionDefinition[]; state: Snapshot}> {
 const health = await readJson("/v1/health");
 if (health.name !== "Himothee Studio" || health.bridge_version !== 1 || !health.ready) {
   throw new Error("Incompatible or unavailable Himothee bridge");
 }
 const [actionResponse, state] = await Promise.all([
   readJson("/v1/actions"), readJson("/v1/state")
 ]);
 if (!Array.isArray(actionResponse.actions) || state?.ready !== true) {
   throw new Error("Himothee action registry not ready");
 }
 return {actions: actionResponse.actions, state};
}
export async function execute(actionId: string, params: Parameters): Promise<ExecuteResult> {
 const reply = await readJson("/v1/execute", {
   method: "POST",
   headers: {"Content-Type":"application/json", "X-Himothee-Client":"stream-deck"},
   body: JSON.stringify({action_id: actionId, params})
 });
 if (typeof reply?.success !== "boolean") throw new Error("Malformed action result");
 return reply as ExecuteResult;
}
