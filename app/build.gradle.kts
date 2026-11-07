plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)

}

android {
    namespace = "my.opengl.testsdl"
    buildToolsVersion = "36.0.0"
    compileSdk = 35

    defaultConfig {
        applicationId = "my.opengl.testsdl"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters.add("arm64-v8a")
            abiFilters.add( "x86_64")
            abiFilters.add("armeabi-v7a")
            abiFilters.add("x86")

        }

    }
    buildFeatures {
        prefab = true

    }


    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11

    }
    kotlinOptions {
        jvmTarget = "11"
    }
    externalNativeBuild {
        ndkVersion = "29.0.13113456"
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.0.2"
        }
    }
    buildFeatures {
        viewBinding = true
    }
}

dependencies {

    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    implementation(files("libs/SDL3-3.2.4.aar"))
    implementation(files("libs/SDL3_image-3.2.0.aar"))
    implementation(libs.activity)

    implementation(files("libs/SDL3_ttf-3.2.0.aar"))

    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)


}