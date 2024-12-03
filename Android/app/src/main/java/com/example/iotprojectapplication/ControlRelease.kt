package com.example.iotprojectapplication

data class ControlRelease(
    val state: ReleaseState
)

data class ReleaseState(
    val desired: ReleaseDesired
)

data class ReleaseDesired(
    val flag: Int
)
