object ProjectInfo {
    const val NAMESPACE = "my.opengl.testsdl"
    const val APPLICATION_ID = "my.opengl.testsdl"

    // SDK & Build Tool Configurations
    const val COMPILE_SDK = 36
    const val MIN_SDK = 26
    const val TARGET_SDK = 36
    const val BUILD_TOOLS_VERSION = "36.0.0"

    // Versioning details
    const val VERSION_MAJOR = 1
    const val VERSION_MINOR = 0
    const val VERSION_PATCH = 0
    const val VERSION_RC = 0 // 0 = final release; >0 = release candidate number (e.g. 1 for rc1)

    val versionCode: Int
        get() = (VERSION_MAJOR * 10000) + (VERSION_MINOR * 100) + VERSION_PATCH

    val versionName: String
        get() = if (VERSION_RC > 0) {
            "$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-rc$VERSION_RC"
        } else {
            "$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"
        }

    // Native & NDK Configurations
    const val NDK_VERSION = "29.0.13113456"
    const val CMAKE_VERSION = "4.0.2"
    val abiFilters = listOf("armeabi-v7a", "arm64-v8a", "x86_64", "x86")

    const val TEST_INSTRUMENTATION_RUNNER = "androidx.test.runner.AndroidJUnitRunner"
}
