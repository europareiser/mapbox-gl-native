# Mapbox ProGuard configuration is handled in the SDK
-dontwarn android.support.**
-dontwarn java.lang.**
-dontwarn org.codehaus.**
-keep class com.google.**
-dontwarn com.google.**

-keep class com.mapbox.mapboxsdk.testapp.model.customlayer.ExampleCustomLayer { *; }
