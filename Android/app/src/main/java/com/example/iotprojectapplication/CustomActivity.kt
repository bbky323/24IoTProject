package com.example.iotprojectapplication

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory

class CustomActivity : AppCompatActivity() {

    private lateinit var cname1: EditText
    private lateinit var ctemp1: EditText
    private lateinit var chumid1: EditText
    private lateinit var cname2: EditText
    private lateinit var ctemp2: EditText
    private lateinit var chumid2: EditText
    private lateinit var cname3: EditText
    private lateinit var ctemp3: EditText
    private lateinit var chumid3: EditText
    private lateinit var cname4: EditText
    private lateinit var ctemp4: EditText
    private lateinit var chumid4: EditText

    private lateinit var nameStatus: EditText
    private lateinit var tempStatus: EditText
    private lateinit var humidStatus: EditText

    private lateinit var applyButton: Button
    private lateinit var cancelButton: Button

    private val retrofit = Retrofit.Builder()
        .baseUrl("${IoTAPIEndpoint}")
        .addConverterFactory(GsonConverterFactory.create())
        .build()

    private val retrofit2 = Retrofit.Builder()
        .baseUrl("${IoT_DB_API_Endpoint}")
        .addConverterFactory(GsonConverterFactory.create())
        .build()

    private val apiService = retrofit.create(ApiService::class.java)
    private val apiService2 = retrofit2.create(ApiService::class.java)
    private val coroutineScope = CoroutineScope(Dispatchers.Main + Job())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_custom)

        initializeViews()
        startPeriodicSensorUpdate()

        applyButton.setOnClickListener {
            showConfirmationDialog(
                "커스텀 설정 변경",
                "커스텀을 변경하시겠습니까?",
                onConfirm = {
                    updateCustomStatus()
                }
            )
        }

        cancelButton.setOnClickListener {
            startActivity(Intent(this, MainActivity::class.java))
        }

    }

    private fun initializeViews() {
        cname1 = findViewById(R.id.cname_1)
        ctemp1 = findViewById(R.id.ctemp_1)
        chumid1 = findViewById(R.id.chumid_1)
        cname2 = findViewById(R.id.cname_2)
        ctemp2 = findViewById(R.id.ctemp_2)
        chumid2 = findViewById(R.id.chumid_2)
        cname3 = findViewById(R.id.cname_3)
        ctemp3 = findViewById(R.id.ctemp_3)
        chumid3 = findViewById(R.id.chumid_3)
        cname4 = findViewById(R.id.cname_4)
        ctemp4 = findViewById(R.id.ctemp_4)
        chumid4 = findViewById(R.id.chumid_4)

        nameStatus = findViewById(R.id.name_status)
        tempStatus = findViewById(R.id.temp_status)
        humidStatus = findViewById(R.id.humid_status)

        applyButton = findViewById(R.id.manual_operation_button)
        cancelButton = findViewById(R.id.manual_operation_cancel_button)

        setInputFieldsReadOnly()
    }

    private fun setInputFieldsReadOnly() {
        val inputs = listOf(cname1, ctemp1, chumid1, cname2, ctemp2, chumid2, cname3, ctemp3, chumid3, cname4, ctemp4, chumid4)
        inputs.forEach { input ->
            input.isFocusable = false
            input.isClickable = false
        }
    }

    private fun startPeriodicSensorUpdate() {
        coroutineScope.launch {
            try {
                updateSensorData() // 한 번만 호출
            } catch (e: Exception) {
                Log.e("CustomActivity", "Error updating sensor data", e)
                showError("커스텀 데이터 업데이트 실패")
            }
            try {
                updateAvgCustomData() // 한 번만 호출
            } catch (e: Exception) {
                Log.e("CustomActivity", "Error updating sensor data", e)
                showError("센서 데이터 업데이트 실패")
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
        nameStatus.setText(sensorData.state.desired.cname)
        tempStatus.setText(String.format("%.1f", sensorData.state.desired.ctemp))
        humidStatus.setText(String.format("%.1f", sensorData.state.desired.chumid))
    }

    private suspend fun updateAvgCustomData() {
        withContext(Dispatchers.IO) {
            try {
                val avgCustomDataList: List<AvgCustomData> = apiService2.getCustomAvgData()
                withContext(Dispatchers.Main) {
                    updateAvgCustomUI(avgCustomDataList)
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    showError("센서 데이터 가져오기 실패")
                }
            }
        }
    }

    private fun updateAvgCustomUI(avgCustomDataList: List<AvgCustomData>) {
        var i = 0
        for (avgCustomData in avgCustomDataList) {
            val avgTempS = avgCustomData.avgCtemp - 2
            val avgTempE = avgCustomData.avgCtemp + 2
            val avgHumidS = avgCustomData.avgChumid
            val avgHumidE = minOf(avgCustomData.avgChumid + 30, 100)
            if (i == 0) {
                cname1.setText(avgCustomData.cname)
                ctemp1.setText("적정 온도: " + avgTempS.toString() + "-" + avgTempE.toString() + "°C")
                chumid1.setText("적정 습도: " + avgHumidS.toString() + "-" + avgHumidE.toString() +"%")
            }
            else if (i == 1) {
                cname2.setText(avgCustomData.cname)
                ctemp2.setText("적정 온도: " + avgTempS.toString() + "-" + avgTempE.toString() + "°C")
                chumid2.setText("적정 습도: " + avgHumidS.toString() + "-" + avgHumidE.toString() +"%")
            }
            else if (i == 2) {
                cname3.setText(avgCustomData.cname)
                ctemp3.setText("적정 온도: " + avgTempS.toString() + "-" + avgTempE.toString() + "°C")
                chumid3.setText("적정 습도: " + avgHumidS.toString() + "-" + avgHumidE.toString() +"%")
            }
            else {
                cname4.setText(avgCustomData.cname)
                ctemp4.setText("적정 온도: " + avgTempS.toString() + "-" + avgTempE.toString() + "°C")
                chumid4.setText("적정 습도: " + avgHumidS.toString() + "-" + avgHumidE.toString() +"%")
            }
            i++
        }
    }

    private fun updateCustomStatus() {
        val nameValue = nameStatus.text.toString()
        val tempValue = tempStatus.text.toString().toFloatOrNull() ?: 0.0f // 기본값 0
        val humidValue = humidStatus.text.toString().toFloatOrNull() ?: 0.0f // 기본값 0

        val customStatus = CustomStatus(
            state = CustomState(
                desired = CustomDesired(
                    cname = nameValue,
                    ctemp = tempValue,
                    chumid = humidValue
                )
            )
        )

        coroutineScope.launch {
            try {
                val response = withContext(Dispatchers.IO) {
                    apiService.updateCustoms(customStatus)
                }

                if (!response.isSuccessful) {
                    showError("커스텀 업데이트 실패")
                }
            } catch (e: Exception) {
                showError("네트워크 오류")
            }
            try {
                val response = withContext(Dispatchers.IO) {
                    apiService2.updateDB(customStatus)
                }

                if (!response.isSuccessful) {
                    showError("DB 업데이트 실패")
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