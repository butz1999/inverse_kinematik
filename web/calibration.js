const jointForm = document.querySelector("#joint-form");
const pwmForm = document.querySelector("#pwm-form");
const statusBox = document.querySelector("#status");
const baseUrlInput = document.querySelector("#base-url");
const initButton = document.querySelector("#init-button");
const sendJointButton = document.querySelector("#send-joint-button");
const sendButton = document.querySelector("#send-button");

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

function readPwmState() {
  return readNumericState(pwmForm, (value) => Number.parseInt(value, 10));
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
  updateJointForm(body.jointState);
  updatePwmForm(body.jointPwmState);
}

async function postJson(path, payload) {
  const response = await fetch(apiUrl(path), {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: payload ? JSON.stringify(payload) : undefined,
  });

  const text = await response.text();
  const body = text ? JSON.parse(text) : {};
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}: ${text}`);
  }
  return body;
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

pwmForm.addEventListener("change", async (event) => {
  if (!(event.target instanceof HTMLInputElement) || event.target.name === "") {
    return;
  }

  try {
    await sendPwmState(`Changed ${event.target.name}, sending`);
  } catch (error) {
    setStatus(`Send failed: ${error.message}`);
  }
});

jointForm.addEventListener("change", async (event) => {
  if (!(event.target instanceof HTMLInputElement) || event.target.name === "") {
    return;
  }

  try {
    await sendJointState(`Changed ${event.target.name}, sending`);
  } catch (error) {
    setStatus(`Send failed: ${error.message}`);
  }
});
