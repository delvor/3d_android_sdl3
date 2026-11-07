import com.android.build.api.dsl.ApplicationExtension

plugins {
    alias(libs.plugins.android.application)
}

configure<ApplicationExtension> {

    namespace = ProjectInfo.NAMESPACE
    buildToolsVersion = ProjectInfo.BUILD_TOOLS_VERSION
    compileSdk = ProjectInfo.COMPILE_SDK

    defaultConfig {
        applicationId = ProjectInfo.APPLICATION_ID
        minSdk = ProjectInfo.MIN_SDK
        //noinspection OldTargetApi,EditedTargetSdkVersion
        targetSdk = ProjectInfo.TARGET_SDK
        versionCode = ProjectInfo.versionCode
        versionName = ProjectInfo.versionName

        testInstrumentationRunner = ProjectInfo.TEST_INSTRUMENTATION_RUNNER

        ndk {
            abiFilters.clear()
            abiFilters.addAll(ProjectInfo.abiFilters)
        }
    }

    buildFeatures {
        prefab = true
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    externalNativeBuild {
        ndkVersion = ProjectInfo.NDK_VERSION
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = ProjectInfo.CMAKE_VERSION
        }
    }
    buildFeatures {
        viewBinding = true
    }
}

kotlin {
    compilerOptions {
        jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17)
    }
}

dependencies {

    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    implementation(libs.activity)

    //SDL3
    implementation(files("libs/SDL3-3.4.20.aar"))
    implementation(files("libs/SDL3_image-3.4.8.aar"))
    implementation(files("libs/SDL3_ttf-3.2.2.aar"))

    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}
