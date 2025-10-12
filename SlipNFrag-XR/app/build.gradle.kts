import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.jetbrains.kotlin.android)
}

android {
    namespace = "com.heribertodelgado.slipnfrag_xr"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.heribertodelgado.slipnfrag_xr"
        minSdk = 29
        //noinspection ExpiredTargetSdkVersion
        targetSdk = 32
        versionCode = 30
        versionName = "1.0.30"
        shaders {
            glslcArgs += listOf("-c", "-g")
        }
        ndk {
            //noinspection ChromeOsAbiSupport
            abiFilters += "arm64-v8a"
        }
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlin {
        compilerOptions {
            jvmTarget = JvmTarget.fromTarget("1.8")
        }
    }
    buildFeatures {
        prefab = true
        shaders = true
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }
    sourceSets {
        named("main") {
            shaders.srcDir("../../shaders/")
        }
    }
    ndkVersion = "29.0.14206865"
    buildToolsVersion = "36.1.0"
    compileSdkMinor = 1
}

dependencies {

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.games.activity)
    implementation(libs.openxr.loader.android)
}