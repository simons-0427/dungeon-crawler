import java.util.Properties

plugins {
    id("com.android.application")
}

val keystorePropertiesFile = file(System.getenv("DC_KEYSTORE_PROPERTIES") ?: rootProject.file("keystore/keystore.properties").absolutePath)
val keystoreProperties = Properties()
if (keystorePropertiesFile.exists()) {
    keystorePropertiesFile.inputStream().use { keystoreProperties.load(it) }
}

android {
    namespace = "com.dungeoncrawler.playfree"
    compileSdk {
        version = release(36) {
            minorApiLevel = 1
        }
    }
    ndkVersion = "28.2.13676358"

    defaultConfig {
        applicationId = "com.dungeoncrawler.playfree"
        minSdk = 26
        targetSdk = 36
        versionCode = 23
        versionName = "1.7.6"

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

    flavorDimensions += "profile"
    productFlavors {
        create("normal") {
            dimension = "profile"
        }
    }

    signingConfigs {
        if (keystorePropertiesFile.exists()) {
            create("release") {
                storeFile = file(System.getenv("DC_KEYSTORE_ROOT")?.let { "$it/${keystoreProperties.getProperty("storeFile")}" } ?: rootProject.file(checkNotNull(keystoreProperties.getProperty("storeFile"))).absolutePath)
                storePassword = keystoreProperties.getProperty("storePassword")
                keyAlias = keystoreProperties.getProperty("keyAlias")
                keyPassword = keystoreProperties.getProperty("keyPassword")
            }
        }
    }

    buildTypes {
        debug {
            isJniDebuggable = true
        }
        release {
            isMinifyEnabled = false
            ndk {
                debugSymbolLevel = "SYMBOL_TABLE"
            }
            if (keystorePropertiesFile.exists()) {
                signingConfig = signingConfigs.getByName("release")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    sourceSets["main"].assets.srcDir("src/main/assets")

    packaging {
        jniLibs.useLegacyPackaging = false
    }
}
