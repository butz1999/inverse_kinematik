(() => {
  function shouldUpdateControllerMotionForms({
    nowMs,
    force,
    responseGeneration,
    currentGeneration,
    suppressUntilMs,
    lastUpdateMs,
    minimumIntervalMs,
  }) {
    if (responseGeneration !== currentGeneration || nowMs < suppressUntilMs) {
      return false;
    }

    return force || nowMs - lastUpdateMs >= minimumIntervalMs;
  }

  function isCurrentControllerPollingGeneration(responseGeneration, currentGeneration) {
    return responseGeneration === currentGeneration;
  }

  function isReportableTargetPose(pose) {
    const fields = ["x_mm", "y_mm", "z_mm", "p_deg", "r_deg", "g_pct"];
    return fields.every((field) => typeof pose?.[field] === "number" && Number.isFinite(pose[field])) &&
           pose.g_pct >= 0 && pose.g_pct <= 100;
  }

  function poseAfterRejectedMotion(lastConfirmedPose) {
    return isReportableTargetPose(lastConfirmedPose) ? { ...lastConfirmedPose } : null;
  }

  globalThis.controllerReporting = {
    isCurrentControllerPollingGeneration,
    isReportableTargetPose,
    poseAfterRejectedMotion,
    shouldUpdateControllerMotionForms,
  };
})();
