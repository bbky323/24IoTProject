package com.example.iotprojectapplication

data class CustomStatus(
    val state: CustomState
)

data class CustomState(
    val desired: CustomDesired
)

data class CustomDesired(
    val cname: String,
    val ctemp: Float,
    val chumid: Float
)