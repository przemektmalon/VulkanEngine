set(FILES 
        "Asset.hpp"
        "AssetStore.hpp"
        "Camera.hpp"
        "Clock.hpp"
        "Console.hpp"
        "Engine.hpp"
        "EngineConfig.hpp"
        "Event.hpp"
        "File.hpp"
        "Font.hpp"
        "Image.hpp"
        "Keyboard.hpp"
        "Lights.hpp"
        "Material.hpp"
        "Model.hpp"
        "Mouse.hpp"
        "PBRImage.hpp"
        "PCH.hpp"
        "PhysicsObject.hpp"
        "PhysicsWorld.hpp"
        "Profiler.hpp"
        "Rect.hpp"
        "Renderer.hpp"
        "Scripting.hpp"
        "ShaderProgram.hpp"
        "ShaderSpecs.hpp"
        "Texture.hpp"
        "Threading.hpp"
        "Time.hpp"
        "Transform.hpp"
        "Types.hpp"
        "UIElement.hpp"
        "UIElementGroup.hpp"
        "UIPolygon.hpp"
        "UIRenderer.hpp"
        "UIText.hpp"
        "Window.hpp"
        "World.hpp"
        "XMLParser.hpp")


foreach(source_file ${FILES})
    list(APPEND INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}\;")
endforeach()
    
set(INCLUDES ${INCLUDES} PARENT_SCOPE)