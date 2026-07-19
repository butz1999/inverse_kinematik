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
const poseHistoryList = document.querySelector("#pose-history");

const poseFields = ["x_mm", "y_mm", "z_mm", "p_deg", "r_deg", "g_pct"];
const poseHistoryStorageKey = "dilbert.poseHistory.v1";
const committedFormStateSyncers = [];
let poseHistory = loadPoseHistory();

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

function normalizePose(pose) {
  const normalized = {};
  for (const field of poseFields) {
    normalized[field] = roundedNumber(pose[field]);
  }
  return normalized;
}

function poseKey(pose) {
  return poseFields.map((field) => String(roundedNumber(pose[field]))).join("|");
}

function loadPoseHistory() {
  try {
    const parsed = JSON.parse(window.localStorage.getItem(poseHistoryStorageKey) || "[]");
    return Array.isArray(parsed) ? parsed.slice(0, 10).map(normalizePose) : [];
  } catch {
    return [];
  }
}

function savePoseHistory() {
  window.localStorage.setItem(poseHistoryStorageKey, JSON.stringify(poseHistory));
}

function formatPose(pose) {
  return `x ${pose.x_mm}, y ${pose.y_mm}, z ${pose.z_mm}, p ${pose.p_deg}, r ${pose.r_deg}, g ${pose.g_pct}`;
}

function renderPoseHistory() {
  poseHistoryList.replaceChildren();

  for (const pose of poseHistory) {
    const item = document.createElement("li");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = formatPose(pose);
    button.addEventListener("click", () => {
      updatePoseForm(pose);
      setStatus("Pose loaded from history.", pose);
    });
    button.addEventListener("dblclick", async () => {
      updatePoseForm(pose);
      try {
        await sendPoseState("Sending history");
      } catch (error) {
        setStatus(`Send failed: ${error.message}`);
      }
    });
    item.append(button);
    poseHistoryList.append(item);
  }
}

function rememberPose(pose) {
  if (!pose) {
    return;
  }

  const normalized = normalizePose(pose);
  const key = poseKey(normalized);
  poseHistory = [normalized, ...poseHistory.filter((entry) => poseKey(entry) !== key)].slice(0, 10);
  savePoseHistory();
  renderPoseHistory();
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
  return readNumericState(poseForm, Number.parseFloat);
}

function readPwmState() {
  return readNumericState(pwmForm, (value) => Number.parseInt(value, 10));
}

function updatePoseForm(targetPose) {
  if (!targetPose) {
    return;
  }

  for (const [name, value] of Object.entries(targetPose)) {
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

async function sendPoseState(source) {
  const state = {
    ...readPoseState(),
    motionProfile: {
      type: motionProfileTypeSelect.value,
    },
  };
  sendPoseButton.disabled = true;
  try {
    setStatus(`${source} pose...`, state);
    const body = await postJson("/api/motion", state);
    updateFormsFromResponse(body);
    rememberPose(body.targetPose);
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

function addCommittedNumberSend(form, sendState) {
  let inFlight = false;
  let pending = false;
  let lastSentStateKey = formStateKey(form);

  committedFormStateSyncers.push(() => {
    lastSentStateKey = formStateKey(form);
  });

  async function sendLatestState() {
    if (!form.checkValidity()) {
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
      } while (pending && form.checkValidity());
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

addCommittedNumberSend(jointForm, sendJointState);
addCommittedNumberSend(pwmForm, sendPwmState);
renderPoseHistory();
