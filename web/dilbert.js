const poseForm = document.querySelector("#pose-form");
const jointForm = document.querySelector("#joint-form");
const pwmForm = document.querySelector("#pwm-form");
const statusBox = document.querySelector("#status");
const baseUrlInput = document.querySelector("#base-url");
const initButton = document.querySelector("#init-button");
const sendPoseButton = document.querySelector("#send-pose-button");
const sendJointButton = document.querySelector("#send-joint-button");
const sendButton = document.querySelector("#send-button");
const motionProfileTypeSelect = document.querySelector("#motion-profile-type");
const motionProfileVelocityInput = document.querySelector("#motion-profile-velocity");
const motionProfileSampleTimeInput = document.querySelector("#motion-profile-sample-time");
const poseHistoryList = document.querySelector("#pose-history");
const clearButton = document.querySelector("#clear-pose-history-button");
const loadPoseHistoryButton = document.querySelector("#load-pose-history-button");
const savePoseHistoryButton = document.querySelector("#save-pose-history-button");
const poseHistoryFileInput = document.querySelector("#pose-history-file");
const sequenceWaitInput = document.querySelector("#sequence-wait");
const sequenceLedColorKindSelect = document.querySelector("#sequence-led-color-kind");
const sequenceStatusColorSelect = document.querySelector("#sequence-status-color");
const sequenceColorRInput = document.querySelector("#sequence-color-r");
const sequenceColorGInput = document.querySelector("#sequence-color-g");
const sequenceColorBInput = document.querySelector("#sequence-color-b");
const sequenceLedModeSelect = document.querySelector("#sequence-led-mode");
const sequenceLedIntervalInput = document.querySelector("#sequence-led-interval");
const addPoseStepButton = document.querySelector("#add-pose-step-button");
const addWaitStepButton = document.querySelector("#add-wait-step-button");
const addColorStepButton = document.querySelector("#add-color-step-button");
const loadSequenceButton = document.querySelector("#load-sequence-button");
const saveSequenceButton = document.querySelector("#save-sequence-button");
const sequenceFileInput = document.querySelector("#sequence-file");
const startSequenceButton = document.querySelector("#start-sequence-button");
const stopSequenceButton = document.querySelector("#stop-sequence-button");
const sequenceStatusButton = document.querySelector("#sequence-status-button");
const clearSequenceButton = document.querySelector("#clear-sequence-button");
const sequenceStepsList = document.querySelector("#sequence-steps");
const poseFields = ["x_mm", "y_mm", "z_mm", "p_deg", "r_deg", "g_pct"];
const maxPoseHistoryEntries = 10;
const maxSequenceSteps = 16;
const poseHistoryFileVersion = 1;
const sequenceFileVersion = 1;
const poseHistoryStorageKey = "dilbert.poseHistory.v1";
const sequenceStorageKey = "dilbert.sequence.v1";
const committedFormStateSyncers = [];
let poseHistory = loadPoseHistory();
let sequenceSteps = loadSequence();
let sequenceStatusTimer = 0;

function apiUrl(path) {
  const baseUrl = baseUrlInput.value.replace(/\/+$/, "");
  return `${baseUrl}${path}`;
}

function setStatus(message, payload) {
  statusBox.textContent = payload ? `${message}\n${JSON.stringify(payload, null, 2)}` : message;
}

function roundedNumber(value) {
  return Number.parseFloat(Number(value).toFixed(3));
}

function normalizePoseName(name) {
  return String(name ?? "").trim().slice(0, 48);
}

function normalizePose(pose) {
  const normalized = {};
  for (const field of poseFields) {
    normalized[field] = roundedNumber(pose[field]);
  }
  return normalized;
}

function isValidPose(pose) {
  return poseFields.every((field) => Number.isFinite(Number(pose?.[field])));
}

function poseKey(pose) {
  return poseFields.map((field) => String(roundedNumber(pose[field]))).join("|");
}

function poseFromEntry(entry) {
  return entry?.pose ?? entry;
}

function normalizePoseHistoryEntry(entry) {
  const pose = poseFromEntry(entry);
  if (!isValidPose(pose)) {
    return null;
  }

  return {
    name: normalizePoseName(entry?.name),
    pose: normalizePose(pose),
  };
}

function poseHistoryEntryKey(entry) {
  return poseKey(entry.pose);
}

function normalizePoseHistory(poses) {
  if (!Array.isArray(poses)) {
    return [];
  }

  const normalized = [];
  const seenKeys = new Set();
  for (const entry of poses) {
    const normalizedEntry = normalizePoseHistoryEntry(entry);
    if (!normalizedEntry) {
      continue;
    }

    const key = poseHistoryEntryKey(normalizedEntry);
    if (seenKeys.has(key)) {
      continue;
    }

    normalized.push(normalizedEntry);
    seenKeys.add(key);
    if (normalized.length >= maxPoseHistoryEntries) {
      break;
    }
  }

  return normalized;
}

function poseHistoryDocument() {
  return {
    version: poseHistoryFileVersion,
    poses: poseHistory,
  };
}

function parsePoseHistoryDocument(document) {
  if (Array.isArray(document)) {
    return normalizePoseHistory(document);
  }

  if (!document || document.version !== poseHistoryFileVersion) {
    throw new Error(`Unsupported pose history file version: ${document?.version ?? "missing"}`);
  }

  return normalizePoseHistory(document.poses);
}

function loadPoseHistory() {
  try {
    const parsed = JSON.parse(window.localStorage.getItem(poseHistoryStorageKey) || "[]");
    return parsePoseHistoryDocument(parsed);
  } catch {
    return [];
  }
}

function savePoseHistory() {
  window.localStorage.setItem(poseHistoryStorageKey, JSON.stringify(poseHistoryDocument()));
}

function normalizeSequenceType(type) {
  return type === "rgb_color" ? "led" : (type ?? "pose");
}

function normalizeLedColorKind(kind) {
  return ["status", "rgb"].includes(kind) ? kind : "rgb";
}

function loadSequence() {
  try {
    const parsed = JSON.parse(window.localStorage.getItem(sequenceStorageKey) || "[]");
    return normalizeSequence(parsed);
  } catch {
    return [];
  }
}

function saveSequence() {
  window.localStorage.setItem(sequenceStorageKey, JSON.stringify(sequenceSteps));
}

function normalizeSequence(steps) {
  if (!Array.isArray(steps)) {
    return [];
  }

  return steps
    .map((step) => {
      const type = normalizeSequenceType(step?.type);
      const name = normalizePoseName(step?.name);

      if (type === "led") {
        return {
          type,
          name,
          colorKind: normalizeLedColorKind(step?.colorKind),
          statusColor: step?.statusColor ?? "green",
          r: Math.max(0, Math.min(255, Number.parseInt(step?.r ?? 0, 10) || 0)),
          g: Math.max(0, Math.min(255, Number.parseInt(step?.g ?? 0, 10) || 0)),
          b: Math.max(0, Math.min(255, Number.parseInt(step?.b ?? 0, 10) || 0)),
          mode: step?.mode ?? "on",
          interval_ms: Math.max(1, Number.parseInt(step?.interval_ms ?? 500, 10) || 500),
        };
      }

      if (type === "wait") {
        return {
          type,
          name,
          wait_ms: Math.max(0, Number.parseInt(step?.wait_ms ?? 0, 10) || 0),
        };
      }

      return {
        type,
        name,
        pose: normalizePose(poseFromEntry(step)),
        motionProfile: step?.motionProfile ?? readMotionProfileState(),
      };
    })
    .filter((step) => step.type === "led" || step.type === "wait" || isValidPose(step.pose))
    .slice(0, maxSequenceSteps);
}

function sequenceDocument() {
  return {
    version: sequenceFileVersion,
    steps: sequenceSteps,
  };
}

function parseSequenceDocument(document) {
  if (Array.isArray(document)) {
    return normalizeSequence(document);
  }

  if (!document || document.version !== sequenceFileVersion) {
    throw new Error(`Unsupported sequence file version: ${document?.version ?? "missing"}`);
  }

  return normalizeSequence(document.steps);
}

function setPoseHistory(nextPoseHistory) {
  poseHistory = normalizePoseHistory(nextPoseHistory);
  savePoseHistory();
  renderPoseHistory();
}

function formatPose(pose) {
  return `x ${pose.x_mm}, y ${pose.y_mm}, z ${pose.z_mm}, p ${pose.p_deg}, r ${pose.r_deg}, g ${pose.g_pct}`;
}

function formatMotionProfile(motionProfile) {
  if (!motionProfile) {
    return "";
  }

  return `profile ${motionProfile.type}, v ${motionProfile.target_velocity_deg_s}, sample ${motionProfile.sample_time_ms}`;
}

function formatSequencePoseStep(step) {
  const motionProfile = formatMotionProfile(step.motionProfile);
  return motionProfile ? `${formatPose(step.pose)}, ${motionProfile}` : formatPose(step.pose);
}

function readPoseName() {
  return normalizePoseName(poseForm.elements.namedItem("name")?.value);
}

function poseHistoryEntryFromPose(pose, name = readPoseName()) {
  return {
    name: normalizePoseName(name),
    pose: normalizePose(pose),
  };
}

function renderPoseHistory() {
  poseHistoryList.replaceChildren();

  for (const entry of poseHistory) {
    const item = document.createElement("li");
    const row = document.createElement("div");
    row.className = "pose-history-row";

    // Button to repeat pose
    const button = document.createElement("button");
    button.type = "button";
    const name = document.createElement("span");
    name.className = "pose-history-name";
    name.textContent = entry.name || "Unnamed pose";
    const values = document.createElement("span");
    values.className = "pose-history-values";
    values.textContent = formatPose(entry.pose);
    button.append(name, values);
    button.addEventListener("click", () => {
      updatePoseForm(entry);
      setStatus("Pose loaded from history.", entry);
    });
    button.addEventListener("dblclick", async () => {
      updatePoseForm(entry);
      try {
        await sendPoseState("Sending history");
      } catch (error) {
        setStatus(`Send failed: ${error.message}`);
      }
    });

    // Button to delete line
    const deleteButton = document.createElement("button");
    deleteButton.type = "button";
    deleteButton.textContent = "🗑️";
    deleteButton.addEventListener("click", () => {
      deletePoseHistoryEntry(entry);
      setStatus("Pose deleted from history.", entry);
    });

    row.append(button);
    row.append(deleteButton);
    item.append(row);
    poseHistoryList.append(item);
  }
}

function sequenceStepTitle(step, index) {
  return step.name || `Step ${index + 1}`;
}

function sequenceStepValues(step) {
  if (step.type === "led") {
    return formatLedStep(step);
  }

  if (step.type === "wait") {
    return `wait ${step.wait_ms} ms`;
  }

  return formatSequencePoseStep(step);
}

function createSequenceStepButton(step, index) {
  const button = document.createElement("button");
  button.type = "button";

  const name = document.createElement("span");
  name.className = "sequence-step-name";
  name.textContent = sequenceStepTitle(step, index);

  const values = document.createElement("span");
  values.className = "sequence-step-values";
  values.textContent = sequenceStepValues(step);

  button.append(name, values);
  button.addEventListener("click", () => {
    updatePoseForm(step);
    if (step.type === "pose") {
      updateMotionProfileForm(step.motionProfile);
    }
    setStatus("Sequence step loaded.", step);
  });
  return button;
}

function createSequenceStepDeleteButton(index) {
  const deleteButton = document.createElement("button");
  deleteButton.type = "button";
  deleteButton.textContent = "🗑️";
  deleteButton.addEventListener("click", () => {
    sequenceSteps.splice(index, 1);
    saveSequence();
    renderSequence();
    setStatus("Sequence step removed.");
  });
  return deleteButton;
}

function createSequenceStepItem(step, index) {
  const item = document.createElement("li");
  const row = document.createElement("div");
  row.className = "sequence-step-row";

  row.append(createSequenceStepButton(step, index));
  row.append(createSequenceStepDeleteButton(index));
  item.append(row);
  return item;
}

function renderSequence() {
  sequenceStepsList.replaceChildren();

  sequenceSteps.forEach((step, index) => {
    sequenceStepsList.append(createSequenceStepItem(step, index));
  });
}

function deletePoseHistoryEntry(entryToDelete) {
  const key = poseHistoryEntryKey(entryToDelete);
  setPoseHistory(poseHistory.filter((entry) => poseHistoryEntryKey(entry) !== key));
}

function clearPoseHistory() {
  setPoseHistory([]);
  window.localStorage.removeItem(poseHistoryStorageKey);
}

function rememberPose(pose, name = readPoseName()) {
  if (!pose) {
    return;
  }

  const entry = poseHistoryEntryFromPose(pose, name);
  const key = poseHistoryEntryKey(entry);
  setPoseHistory([entry, ...poseHistory.filter((storedEntry) => poseHistoryEntryKey(storedEntry) !== key)]);
}

function poseHistoryFileName() {
  const timestamp = new Date().toISOString().slice(0, 19).replace(/[-:T]/g, "");
  return `dilbert-pose-history-${timestamp}.json`;
}

function poseHistoryJson() {
  return `${JSON.stringify(poseHistoryDocument(), null, 2)}\n`;
}

async function savePoseHistoryFile() {
  const json = poseHistoryJson();
  if ("showSaveFilePicker" in window) {
    const handle = await window.showSaveFilePicker({
      suggestedName: poseHistoryFileName(),
      types: [
        {
          description: "Pose history JSON",
          accept: {
            "application/json": [".json"],
          },
        },
      ],
    });
    const writable = await handle.createWritable();
    await writable.write(json);
    await writable.close();
    return;
  }

  const url = URL.createObjectURL(new Blob([json], { type: "application/json" }));
  const link = document.createElement("a");
  link.href = url;
  link.download = poseHistoryFileName();
  link.click();
  URL.revokeObjectURL(url);
}

async function loadPoseHistoryFile(file) {
  const text = await file.text();
  const importedHistory = parsePoseHistoryDocument(JSON.parse(text));
  setPoseHistory(importedHistory);
}

function sequenceFileName() {
  const timestamp = new Date().toISOString().slice(0, 19).replace(/[-:T]/g, "");
  return `dilbert-sequence-${timestamp}.json`;
}

function sequenceJson() {
  return `${JSON.stringify(sequenceDocument(), null, 2)}\n`;
}

async function saveSequenceFile() {
  const json = sequenceJson();
  if ("showSaveFilePicker" in window) {
    const handle = await window.showSaveFilePicker({
      suggestedName: sequenceFileName(),
      types: [
        {
          description: "Sequence JSON",
          accept: {
            "application/json": [".json"],
          },
        },
      ],
    });
    const writable = await handle.createWritable();
    await writable.write(json);
    await writable.close();
    return;
  }

  const url = URL.createObjectURL(new Blob([json], { type: "application/json" }));
  const link = document.createElement("a");
  link.href = url;
  link.download = sequenceFileName();
  link.click();
  URL.revokeObjectURL(url);
}

async function loadSequenceFile(file) {
  const text = await file.text();
  sequenceSteps = parseSequenceDocument(JSON.parse(text));
  saveSequence();
  renderSequence();
}

function readNumericState(form, parser) {
  const data = new FormData(form);
  const state = {};
  for (const [name, value] of data.entries()) {
    state[name] = parser(value);
  }
  return state;
}

function readJointState() {
  return readNumericState(jointForm, Number.parseFloat);
}

function readPoseState() {
  const state = {};
  for (const field of poseFields) {
    state[field] = Number.parseFloat(poseForm.elements.namedItem(field).value);
  }
  return state;
}

function readMotionProfileState() {
  return {
    type: motionProfileTypeSelect.value,
    target_velocity_deg_s: Number.parseFloat(motionProfileVelocityInput.value),
    sample_time_ms: Number.parseInt(motionProfileSampleTimeInput.value, 10),
  };
}

function updateMotionProfileForm(motionProfile) {
  if (!motionProfile) {
    return;
  }

  motionProfileTypeSelect.value = motionProfile.type;
  motionProfileVelocityInput.value = String(motionProfile.target_velocity_deg_s);
  motionProfileSampleTimeInput.value = String(motionProfile.sample_time_ms);
}

function readSequenceWait() {
  return Math.max(0, Number.parseInt(sequenceWaitInput.value, 10) || 0);
}

function readSequenceColor() {
  const clamp = (value) => Math.max(0, Math.min(255, Number.parseInt(value, 10) || 0));
  return {
    r: clamp(sequenceColorRInput.value),
    g: clamp(sequenceColorGInput.value),
    b: clamp(sequenceColorBInput.value),
  };
}

function readSequenceLedColorState() {
  return {
    colorKind: sequenceLedColorKindSelect.value,
    statusColor: sequenceStatusColorSelect.value,
    ...readSequenceColor(),
  };
}

function readSequenceLedOptions() {
  return {
    mode: sequenceLedModeSelect.value,
    interval_ms: Math.max(1, Number.parseInt(sequenceLedIntervalInput.value, 10) || 500),
  };
}

function formatLedStep(step) {
  const parts = [];
  if (step.colorKind === "status") {
    parts.push(`color ${step.statusColor}`);
  } else {
    parts.push(`rgb ${step.r}, ${step.g}, ${step.b}`);
  }
  if (step.mode) {
    parts.push(step.mode);
  }
  if (step.interval_ms) {
    parts.push(`${step.interval_ms} ms`);
  }
  return parts.join(", ");
}

function isMotionProfileSendable(profile) {
  return Number.isFinite(profile.target_velocity_deg_s) && profile.target_velocity_deg_s > 0.0 &&
         Number.isInteger(profile.sample_time_ms) && profile.sample_time_ms > 0;
}

function readPwmState() {
  return readNumericState(pwmForm, (value) => Number.parseInt(value, 10));
}

function updatePoseForm(targetPose) {
  if (!targetPose) {
    return;
  }

  const entry = targetPose.pose ? targetPose : { pose: targetPose };
  const nameInput = poseForm.elements.namedItem("name");
  if (targetPose.pose && nameInput instanceof HTMLInputElement) {
    nameInput.value = entry.name ?? "";
  }

  for (const [name, value] of Object.entries(entry.pose)) {
    const input = poseForm.elements.namedItem(name);
    if (input instanceof HTMLInputElement) {
      input.value = String(value);
    }
  }
}

function updateJointForm(jointState) {
  if (!jointState) {
    return;
  }

  for (const [name, value] of Object.entries(jointState)) {
    const input = jointForm.elements.namedItem(name);
    if (input instanceof HTMLInputElement) {
      input.value = String(value);
    }
  }
}

function updatePwmForm(jointPwmState) {
  if (!jointPwmState) {
    return;
  }

  for (const [name, value] of Object.entries(jointPwmState)) {
    const input = pwmForm.elements.namedItem(name);
    if (input instanceof HTMLInputElement) {
      input.value = String(value);
    }
  }
}

function updateFormsFromResponse(body) {
  updatePoseForm(body.targetPose);
  updateJointForm(body.jointState);
  updatePwmForm(body.jointPwmState);
  refreshCommittedFormStates();
}

function refreshCommittedFormStates() {
  for (const syncFormState of committedFormStateSyncers) {
    syncFormState();
  }
}

async function postJson(path, payload) {
  const url = apiUrl(path);
  const response = await fetch(url, {
    method: "POST",
    headers: {
      "Accept": "application/json",
      "Content-Type": "text/plain",
    },
    body: payload ? JSON.stringify(payload) : undefined,
  }).catch((error) => {
    throw new Error(`${error.message} (${url})`);
  });

  const text = await response.text();
  const body = text ? JSON.parse(text) : {};
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}: ${text}`);
  }
  return body;
}

async function getJson(path) {
  const url = apiUrl(path);
  const response = await fetch(url, {
    method: "GET",
    headers: {
      "Accept": "application/json",
    },
  }).catch((error) => {
    throw new Error(`${error.message} (${url})`);
  });

  const text = await response.text();
  const body = text ? JSON.parse(text) : {};
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}: ${text}`);
  }
  return body;
}

function ledColorRequestPayload(step) {
  if (step.colorKind === "status") {
    return {
      color: step.statusColor,
    };
  }

  return {
    rgb: {
      r: step.r,
      g: step.g,
      b: step.b,
    },
  };
}

function ledStepRequestPayload(step) {
  return {
    type: "led",
    name: step.name,
    ...ledColorRequestPayload(step),
    mode: step.mode,
    interval_ms: step.interval_ms,
  };
}

function waitStepRequestPayload(step) {
  return {
    type: "wait",
    duration_ms: step.wait_ms,
  };
}

function poseStepRequestPayload(step) {
  return {
    type: "pose",
    name: step.name,
    ...step.pose,
    motionProfile: step.motionProfile,
  };
}

function sequenceStepRequestPayload(step) {
  if (step.type === "led") {
    return ledStepRequestPayload(step);
  }

  if (step.type === "wait") {
    return waitStepRequestPayload(step);
  }

  if (step.type === "pose") {
    return poseStepRequestPayload(step);
  }

  throw new Error(`Unsupported sequence step type: ${step.type}`);
}

function sequenceRequestPayload() {
  return {
    steps: sequenceSteps.map(sequenceStepRequestPayload),
  };
}

function updateSequencePolling(body) {
  const activeStatuses = ["planning", "motion_active", "waiting"];
  const status = body?.sequence?.status;
  if (activeStatuses.includes(status)) {
    if (!sequenceStatusTimer) {
      sequenceStatusTimer = window.setInterval(() => {
        void refreshSequenceStatus();
      }, 1000);
    }
    return;
  }

  if (sequenceStatusTimer) {
    window.clearInterval(sequenceStatusTimer);
    sequenceStatusTimer = 0;
  }
}

function sequenceStatusMessage(body) {
  const status = body?.sequence?.status;
  if (["planning", "motion_active", "waiting"].includes(status)) {
    return "Sequence running.";
  }
  if (status === "completed") {
    return "Sequence finished.";
  }
  if (status === "stopped") {
    return "Sequence stopped.";
  }
  if (status === "failed") {
    return "Sequence failed.";
  }
  return "Sequence status.";
}

async function refreshSequenceStatus() {
  const body = await getJson("/api/sequence/status");
  updateFormsFromResponse(body);
  updateSequencePolling(body);
  setStatus(sequenceStatusMessage(body), body);
}

async function startSequence() {
  if (sequenceSteps.length === 0) {
    setStatus("Sequence is empty.");
    return;
  }

  startSequenceButton.disabled = true;
  try {
    const payload = sequenceRequestPayload();
    setStatus("Starting sequence...", payload);
    const body = await postJson("/api/sequence/start", payload);
    updateFormsFromResponse(body);
    updateSequencePolling(body);
    setStatus("Sequence started.", body);
  } finally {
    startSequenceButton.disabled = false;
  }
}

async function stopSequence() {
  stopSequenceButton.disabled = true;
  try {
    const body = await postJson("/api/sequence/stop");
    updateFormsFromResponse(body);
    updateSequencePolling(body);
    setStatus("Sequence stopped.", body);
  } finally {
    stopSequenceButton.disabled = false;
  }
}

async function sendPoseState(source) {
  const motionProfile = readMotionProfileState();
  if (!isMotionProfileSendable(motionProfile)) {
    setStatus("Send failed: motion profile values must be positive numbers.");
    return;
  }

  const poseName = readPoseName();
  const state = {
    ...readPoseState(),
    motionProfile,
  };
  sendPoseButton.disabled = true;
  try {
    setStatus(`${source} pose...`, state);
    const body = await postJson("/api/motion", state);
    updateFormsFromResponse(body);
    rememberPose(body.targetPose, poseName);
    setStatus("Pose accepted.", body);
  } finally {
    sendPoseButton.disabled = false;
  }
}

async function sendJointState(source) {
  const state = readJointState();
  sendJointButton.disabled = true;
  try {
    setStatus(`${source} position...`, state);
    const body = await postJson("/api/joint-motion", state);
    updateFormsFromResponse(body);
    const fkBody = await postJson("/api/forward-kinematics", body.jointState || state);
    updatePoseForm(fkBody.targetPose);
    if (source !== "Updating") {
      rememberPose(fkBody.targetPose);
    }
    setStatus("Position accepted.", {
      jointMotion: body,
      forwardKinematics: fkBody,
    });
  } finally {
    sendJointButton.disabled = false;
  }
}

async function sendPwmState(source) {
  const state = readPwmState();
  sendButton.disabled = true;
  try {
    setStatus(`${source} PWM...`, state);
    const body = await postJson("/api/joint-pwm-motion", state);
    updateFormsFromResponse(body);
    setStatus("PWM accepted.", body);
  } finally {
    sendButton.disabled = false;
  }
}

function formStateKey(form) {
  return JSON.stringify(readNumericState(form, (value) => value));
}

function isSpinnerPointerEvent(event) {
  const rect = event.currentTarget.getBoundingClientRect();
  return event.clientX >= rect.right - 28;
}

function isCommittedNumberFormSendable(form) {
  for (const input of form.querySelectorAll('input[type="number"]')) {
    const value = Number(input.value);
    if (input.validity.badInput || input.value === "" || !Number.isFinite(value)) {
      return false;
    }

    if (input.min !== "" && value < Number(input.min)) {
      return false;
    }

    if (input.max !== "" && value > Number(input.max)) {
      return false;
    }

    if (input.name.endsWith("_pwm") && !Number.isInteger(value)) {
      return false;
    }
  }

  return true;
}

function addCommittedNumberSend(form, sendState) {
  let inFlight = false;
  let pending = false;
  let lastSentStateKey = formStateKey(form);

  committedFormStateSyncers.push(() => {
    lastSentStateKey = formStateKey(form);
  });

  async function sendLatestState() {
    if (!isCommittedNumberFormSendable(form)) {
      return;
    }

    const stateKey = formStateKey(form);
    if (stateKey === lastSentStateKey) {
      return;
    }

    lastSentStateKey = stateKey;

    if (inFlight) {
      pending = true;
      return;
    }

    inFlight = true;
    try {
      do {
        pending = false;
        await sendState("Updating");
        lastSentStateKey = formStateKey(form);
      } while (pending && isCommittedNumberFormSendable(form));
    } catch (error) {
      setStatus(`Update failed: ${error.message}`);
    } finally {
      inFlight = false;
    }
  }

  for (const input of form.querySelectorAll('input[type="number"]')) {
    let keyboardStepPending = false;
    let spinnerPointerActive = false;
    let tabLeavePending = false;

    input.addEventListener("pointerdown", (event) => {
      spinnerPointerActive = isSpinnerPointerEvent(event);
    });

    input.addEventListener("pointerup", () => {
      spinnerPointerActive = false;
    });

    input.addEventListener("pointercancel", () => {
      spinnerPointerActive = false;
    });

    input.addEventListener("keydown", (event) => {
      keyboardStepPending = ["ArrowUp", "ArrowDown", "PageUp", "PageDown"].includes(event.key);
      tabLeavePending = event.key === "Tab";
    });

    input.addEventListener("input", () => {
      if (spinnerPointerActive || keyboardStepPending) {
        void sendLatestState();
      }
      keyboardStepPending = false;
    });

    input.addEventListener("blur", () => {
      spinnerPointerActive = false;
      if (tabLeavePending) {
        void sendLatestState();
      }
      tabLeavePending = false;
    });
  }
}

initButton.addEventListener("click", async () => {
  try {
    setStatus("Initializing...");
    const body = await postJson("/api/servo-driver/init");
    updateFormsFromResponse(body);
    setStatus("Initialized.", body);
  } catch (error) {
    setStatus(`Init failed: ${error.message}`);
  }
});

sendPoseButton.addEventListener("click", async () => {
  try {
    await sendPoseState("Sending");
  } catch (error) {
    setStatus(`Send failed: ${error.message}`);
  }
});

addPoseStepButton.addEventListener("click", () => {
  const motionProfile = readMotionProfileState();
  if (!isMotionProfileSendable(motionProfile)) {
    setStatus("Add failed: motion profile values must be positive numbers.");
    return;
  }
  if (sequenceSteps.length >= maxSequenceSteps) {
    setStatus(`Add failed: maximum sequence length is ${maxSequenceSteps} steps.`);
    return;
  }

  const step = {
    type: "pose",
    name: readPoseName(),
    pose: normalizePose(readPoseState()),
    motionProfile,
  };
  sequenceSteps.push(step);
  saveSequence();
  renderSequence();
  setStatus("Sequence step added.", step);
});

addWaitStepButton.addEventListener("click", () => {
  if (sequenceSteps.length >= maxSequenceSteps) {
    setStatus(`Add failed: maximum sequence length is ${maxSequenceSteps} steps.`);
    return;
  }

  const step = {
    type: "wait",
    name: "Wait",
    wait_ms: readSequenceWait(),
  };
  sequenceSteps.push(step);
  saveSequence();
  renderSequence();
  setStatus("Wait step added.", step);
});

addColorStepButton.addEventListener("click", () => {
  if (sequenceSteps.length >= maxSequenceSteps) {
    setStatus(`Add failed: maximum sequence length is ${maxSequenceSteps} steps.`);
    return;
  }

  const step = {
    type: "led",
    name: "LED",
    ...readSequenceLedColorState(),
    ...readSequenceLedOptions(),
  };
  sequenceSteps.push(step);
  saveSequence();
  renderSequence();
  setStatus("Color step added.", step);
});

startSequenceButton.addEventListener("click", async () => {
  try {
    await startSequence();
  } catch (error) {
    setStatus(`Sequence start failed: ${error.message}`);
  }
});

stopSequenceButton.addEventListener("click", async () => {
  try {
    await stopSequence();
  } catch (error) {
    setStatus(`Sequence stop failed: ${error.message}`);
  }
});

sequenceStatusButton.addEventListener("click", async () => {
  try {
    await refreshSequenceStatus();
  } catch (error) {
    setStatus(`Sequence status failed: ${error.message}`);
  }
});

clearSequenceButton.addEventListener("click", () => {
  sequenceSteps = [];
  saveSequence();
  renderSequence();
  setStatus("Sequence cleared.");
});

loadSequenceButton.addEventListener("click", () => {
  sequenceFileInput.click();
});

saveSequenceButton.addEventListener("click", async () => {
  try {
    await saveSequenceFile();
    setStatus("Sequence saved.", sequenceDocument());
  } catch (error) {
    setStatus(`Save failed: ${error.message}`);
  }
});

sequenceFileInput.addEventListener("change", async () => {
  const [file] = sequenceFileInput.files;
  sequenceFileInput.value = "";
  if (!file) {
    return;
  }

  try {
    await loadSequenceFile(file);
    setStatus("Sequence loaded.", sequenceDocument());
  } catch (error) {
    setStatus(`Load failed: ${error.message}`);
  }
});

sendJointButton.addEventListener("click", async () => {
  try {
    await sendJointState("Sending");
  } catch (error) {
    setStatus(`Send failed: ${error.message}`);
  }
});

sendButton.addEventListener("click", async () => {
  try {
    await sendPwmState("Sending");
  } catch (error) {
    setStatus(`Send failed: ${error.message}`);
  }
});

clearButton.addEventListener("click", () => {
  clearPoseHistory();
  setStatus("Pose history cleared.");
});

loadPoseHistoryButton.addEventListener("click", () => {
  poseHistoryFileInput.click();
});

savePoseHistoryButton.addEventListener("click", async () => {
  try {
    await savePoseHistoryFile();
    setStatus("Pose history saved.", poseHistoryDocument());
  } catch (error) {
    setStatus(`Save failed: ${error.message}`);
  }
});

poseHistoryFileInput.addEventListener("change", async () => {
  const [file] = poseHistoryFileInput.files;
  poseHistoryFileInput.value = "";
  if (!file) {
    return;
  }

  try {
    await loadPoseHistoryFile(file);
    setStatus("Pose history loaded.", poseHistoryDocument());
  } catch (error) {
    setStatus(`Load failed: ${error.message}`);
  }
});

addCommittedNumberSend(jointForm, sendJointState);
addCommittedNumberSend(pwmForm, sendPwmState);
renderPoseHistory();
renderSequence();
