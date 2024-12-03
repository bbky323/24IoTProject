package com.example.iotprojectapplication

import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.GET
import retrofit2.http.POST
import retrofit2.http.PUT

interface ApiService {
    @GET("shadow")
    suspend fun getSensorData(): SensorData

    @POST("shadow")
    suspend fun updateControls(@Body controls: ControlStatus): Response<Unit>

    @POST("shadow")
    suspend fun releaseControls(@Body controls: ControlRelease): Response<Unit>

    @POST("shadow")
    suspend fun updateCustoms(@Body controls: CustomStatus): Response<Unit>

    @GET("items")
    suspend fun getCustomAvgData(): List<AvgCustomData>

    @PUT("items")
    suspend fun updateDB(@Body controls: CustomStatus): Response<Unit>
}