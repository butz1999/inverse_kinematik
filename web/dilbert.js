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

function apiUrl(path) {
  const baseUrl = baseUrlInput.value.replace(/\/+$/, "");
  return `${baseUrl}${path}`;
}

function setStatus(message, payload) {
  statusBox.textContent = payload ? `${message}\n${JSON.stringify(payload, null, 2)}` : message;
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
    setStatus("Position accepted.", body);
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

function addInstantSend(form, sendState) {
  let inFlight = false;
  let pending = false;

  async function sendLatestState() {
    if (!form.checkValidity()) {
      return;
    }

    if (inFlight) {
      pending = true;
      return;
    }

    inFlight = true;
    try {
      do {
        pending = false;
        await sendState("Updating");
      } while (pending && form.checkValidity());
    } catch (error) {
      setStatus(`Update failed: ${error.message}`);
    } finally {
      inFlight = false;
    }
  }

  form.addEventListener("input", sendLatestState);
  form.addEventListener("change", sendLatestState);
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

addInstantSend(jointForm, sendJointState);
addInstantSend(pwmForm, sendPwmState);
