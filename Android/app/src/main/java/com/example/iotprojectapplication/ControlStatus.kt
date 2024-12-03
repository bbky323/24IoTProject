package com.example.iotprojectapplication

data class ControlStatus(
    val state: ControlState
)

data class ControlState(
    val desired: ControlDesired
)

data class ControlDesired(
    val led: String,
    val heat: String,
    val fan: String,
    val pump: String,
    val flag: Int
)