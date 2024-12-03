package com.example.iotprojectapplication

data class SensorData(
    val state: SensorState,
    val version: Int,
    val timestamp: Long
)

data class SensorState(
    val desired: SensorDesired
)

data class SensorDesired(
    val welcome: String,
    val temp: Float,
    val humid: Float,
    val lux: Float,
    val co2: Float,
    val led: String,
    val heat: String,
    val fan: String,
    val pump: String,
    val cname: String,
    val ctemp: Float,
    val chumid: Float
)