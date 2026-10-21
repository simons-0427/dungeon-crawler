plugins {
    id("com.android.application")
}

android {
    namespace = "de.projectdoubleup.dungeoncrawler"
    compileSdk {
        version = release(36) {
            minorApiLevel = 1
        }
    }
    ndkVersion = "28.2.13676358"

    defaultConfig {
        applicationId = "de.projectdoubleup.dungeoncrawler"
        minSdk = 26
        targetSdk = 36
        versionCode = 15
        versionName = "1.5.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=none",
                    "-DPLATFORM=Android",
                    "-DBUILD_EXAMPLES=OFF",
                    "-DBUILD_GAMES=OFF",
                    "-DBUILD_SHARED_LIBS=OFF",
                    "-DWITH_PIC=ON",
                )
            }
        }
    }

    buildTypes {
        debug {
            isJniDebuggable = true
        }
        release {
            isMinifyEnabled = false
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets["main"].assets.srcDir("src/main/assets")

    packaging {
        jniLibs.useLegacyPackaging = false
    }
}
