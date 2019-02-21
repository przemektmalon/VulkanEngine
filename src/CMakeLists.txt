set(FILES
        "Asset.cpp"
        "AssetStore.cpp"
        "Camera.cpp"
        "Console.cpp"
        "Engine.cpp"
        "EngineConfig.cpp"
        "File.cpp"
        "Font.cpp"
        "GBufferPipeline.cpp"
        "Image.cpp"
        "Keyboard.cpp"
        "Lights.cpp"
        "main.cpp"
        "Material.cpp"
        "Model.cpp"
        "Mouse.cpp"		
        "PBRImage.cpp"
        "PBRPipeline.cpp"
        "PhysicsObject.cpp"
        "PhysicsWorld.cpp"
        "Profiler.cpp"
        "Renderer.cpp"
        "ScreenPipeline.cpp"
        "Scripting.cpp"
        "ShadowPipeline.cpp"
        "SSAOPipeline.cpp"
        "Texture.cpp"
        "Threading.cpp"
        "UIElement.cpp"
        "UIElementGroup.cpp"
        "UIPolygon.cpp"
        "UIRenderer.cpp"
        "UIText.cpp"
        "Window.cpp"
        "World.cpp")


foreach(source_file ${FILES})
	list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}\;")
endforeach()

set(SOURCES ${SOURCES} PARENT_SCOPE)