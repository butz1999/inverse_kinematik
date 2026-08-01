set(srcs
    "bluepad32_app_main.c"
    "main.cpp"
    "application/ApiContracts.cpp"
    "application/ApiJson.cpp"
    "application/FirmwareBootstrap.cpp"
    "application/RestApiServer.cpp"
    "application/RunEngine.cpp"
    "config/RobotSettings.cpp"
    "hardware/HardwareCalibration.cpp"
    "hardware/Pca9685ServoDriver.cpp"
    "hardware/Pca9685ServoDriverConfig.cpp"
    "hardware/SerialLogger.cpp"
    "hardware/StatusLed.cpp"
    "orchestration/ControllerHandler.cpp"
    "orchestration/MotionOrchestrator.cpp"
    "orchestration/MotionProfileGenerator.cpp"
    "robotics/Kinematics.cpp"
    "robotics/RobotModel.cpp"
    "robotics/Validation.cpp")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "."
                       REQUIRES espressif__arduino-esp32 bluepad32 bluepad32_arduino btstack ArduinoJson
                                AdafruitPwmServoDriver)
