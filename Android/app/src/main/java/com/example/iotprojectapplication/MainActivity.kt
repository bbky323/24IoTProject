package com.example.iotprojectapplication

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.Switch
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import androidx.appcompat.app.AlertDialog


// MainActivity.kt
class MainActivity : AppCompatActivity() {
    private lateinit var modeStatus: EditText

    private lateinit var temperatureInput: EditText
    private lateinit var humidityInput: EditText
    private lateinit var lightIntensityInput: EditText
    private lateinit var co2LevelInput: EditText

    private lateinit var lightStatus: EditText
    private lateinit var heaterStatus: EditText
    private lateinit var fanStatus: EditText
    private lateinit var pumpStatus: EditText

    private lateinit var lightSwitch: Switch
    private lateinit var heaterSwitch: Switch
    private lateinit var fanSwitch: Switch
    private lateinit var pumpSwitch: Switch

    private lateinit var manualOperationButton: Button
    private lateinit var manualOperationReleaseButton: Button
    private lateinit var customButton: Button

    private val retrofit = Retrofit.Builder()
        .baseUrl("${IoTAPIEndpoint}")
        .addConverterFactory(GsonConverterFactory.create())
        .build()

    private val apiService = retrofit.create(ApiService::class.java)
    private val coroutineScope = CoroutineScope(Dispatchers.Main + Job())

    private var isManualMode = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initializeViews()
        setupListeners()
        startPeriodicSensorUpdate()

        customButton.setOnClickListener {
            startActivity(Intent(this, CustomActivity::class.java))
        }
    }

    private fun initializeViews() {
        modeStatus = findViewById(R.id.mode_status)
        // 센서 데이터 입력 필드
        temperatureInput = findViewById(R.id.temperature_input)
        humidityInput = findViewById(R.id.humidity)
        lightIntensityInput = findViewById(R.id.light_intensity)
        co2LevelInput = findViewById(R.id.co2_level)

        lightStatus = findViewById(R.id.light_status)
        heaterStatus = findViewById(R.id.heater_status)
        fanStatus = findViewById(R.id.fan_status)
        pumpStatus = findViewById(R.id.pump_status)

        // 제어 스위치
        lightSwitch = findViewById(R.id.light_control)
        heaterSwitch = findViewById(R.id.heater_control)
        fanSwitch = findViewById(R.id.fan_control)
        pumpSwitch = findViewById(R.id.pump_control)

        // 수동 조작 버튼
        manualOperationButton = findViewById(R.id.manual_operation_button)
        manualOperationReleaseButton = findViewById(R.id.manual_operation_release_button)
        customButton = findViewById(R.id.custom_button)

        // EditText를 읽기 전용으로 설정
        setInputFieldsReadOnly()
    }

    private fun setInputFieldsReadOnly() {
        val inputs = listOf(modeStatus, temperatureInput, humidityInput, lightIntensityInput, co2LevelInput, lightStatus, heaterStatus, fanStatus, pumpStatus)
        inputs.forEach { input ->
            input.isFocusable = false
            input.isClickable = false
        }
    }

    private fun setupListeners() {
        manualOperationButton.setOnClickListener {
            showConfirmationDialog(
                "수동 조작 활성화",
                "수동 모드를 활성화하시겠습니까?",
                onConfirm = {
                    isManualMode = true
                    modeStatus.setText("수동")
                    updateControlStatus()
                }
            )
        }

        manualOperationReleaseButton.setOnClickListener {
            showConfirmationDialog(
                "수동 조작 해제",
                "수동 모드를 해제하시겠습니까?",
                onConfirm = {
                    isManualMode = false
                    modeStatus.setText("자동")
                    updateControlRelease()
                }
            )
        }
    }

    private fun startPeriodicSensorUpdate() {
        coroutineScope.launch {
            while (true) {
                try {
                    updateSensorData()
                } catch (e: Exception) {
                    Log.e("MainActivity", "Error updating sensor data", e)
                    showError("센서 데이터 업데이트 실패")
                }
                delay(5000) // 30초마다 업데이트
            }
        }
    }

    private suspend fun updateSensorData() {
        withContext(Dispatchers.IO) {
            try {
                val sensorData = apiService.getSensorData()
                withContext(Dispatchers.Main) {
                    updateSensorUI(sensorData)
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    showError("센서 데이터 가져오기 실패")
                }
            }
        }
    }

    private fun updateSensorUI(sensorData: SensorData) {
        temperatureInput.setText(String.format("%.1f°C", sensorData.state.desired.temp))
        humidityInput.setText(String.format("%.1f%%", sensorData.state.desired.humid))
        lightIntensityInput.setText(String.format("%.0f lux", sensorData.state.desired.lux))
        co2LevelInput.setText(String.format("%.0f ppm", sensorData.state.desired.co2))

        lightStatus.setText(
            if (sensorData.state.desired.led == "ON") "현재 ON" else "현재 OFF"
        )
        heaterStatus.setText(
            if (sensorData.state.desired.heat == "ON") "현재 ON" else "현재 OFF"
        )
        fanStatus.setText(
            if (sensorData.state.desired.fan == "ON") "현재 ON" else "현재 OFF"
        )
        pumpStatus.setText(
            if (sensorData.state.desired.pump == "ON") "현재 ON" else "현재 OFF"
        )
    }
    private fun updateControlStatus() {
        val controlStatus = ControlStatus(
            state = ControlState(
                desired = ControlDesired(
                    led = if (lightSwitch.isChecked) "ON" else "OFF",
                    heat = if (heaterSwitch.isChecked) "ON" else "OFF",
                    fan = if (fanSwitch.isChecked) "ON" else "OFF",
                    pump = if (pumpSwitch.isChecked) "ON" else "OFF",
                    flag = if (isManualMode) 1 else 0
                )
            )
        )

        coroutineScope.launch {
            try {
                val response = withContext(Dispatchers.IO) {
                    apiService.updateControls(controlStatus)
                }

                if (!response.isSuccessful) {
                    showError("제어 상태 업데이트 실패")
                }
            } catch (e: Exception) {
                showError("네트워크 오류")
            }
        }
    }
    private fun updateControlRelease() {
        val controlRelease = ControlRelease(
            state = ReleaseState(
                desired = ReleaseDesired(
                    flag = if (isManualMode) 1 else 0
                )
            )
        )

        coroutineScope.launch {
            try {
                val response = withContext(Dispatchers.IO) {
                    apiService.releaseControls(controlRelease)
                }
                lightSwitch.isChecked = false
                heaterSwitch.isChecked = false
                fanSwitch.isChecked = false
                pumpSwitch.isChecked = false

                if (!response.isSuccessful) {
                    showError("제어 상태 업데이트 실패")
                }
            } catch (e: Exception) {
                showError("네트워크 오류")
            }
        }
    }

    private fun showConfirmationDialog(title: String, message: String, onConfirm: () -> Unit) {
        AlertDialog.Builder(this)
            .setTitle(title)
            .setMessage(message)
            .setPositiveButton("확인") { _, _ -> onConfirm() }
            .setNegativeButton("취소", null)
            .show()
    }

    private fun showError(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }

    override fun onDestroy() {
        super.onDestroy()
        coroutineScope.cancel()
    }
}