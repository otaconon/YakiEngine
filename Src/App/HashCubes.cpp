#include "HashCubes.h"

constexpr uint32_t numDirectionalLights = 1;
constexpr uint32_t numPointLights = 0;

HashCubes::HashCubes()
    : m_ctx{m_window.window()},
    m_renderer{m_window.window(), &m_ctx},
    m_materialBuilder{&m_ctx}
{
    m_materialBuilder.BuildPipelines(m_renderer.GetSwapchain(), m_renderer.GetSceneDataDescriptorLayout());
    AssetMngr::Initialize(&m_ctx, &m_materialBuilder);

    initEcs();

    auto& ecs = Ecs::GetInstance();

	m_allMeshes = AssetMngr::LoadGltf("Assets/meshes/basicmesh.glb");
    m_cubeMesh = std::next(m_allMeshes->meshes.begin(), 2)->second;

    Hori::Entity camera = ecs.CreateEntity();
    ecs.AddComponents(camera, Camera{}, Controller{});
    ecs.AddComponents(camera, Translation{{0, -10.f, -10.f}}, Rotation{}, Scale{}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Parent{}, Children{});

    auto& lightData = m_renderer.GetGpuLightData();
    lightData.numDirectionalLights = numDirectionalLights;
    lightData.numPointLights = numPointLights;

    // Create directional lights
    std::array<Hori::Entity, numDirectionalLights> directionalLights;
    for (auto& e : directionalLights)
    {
        e = ecs.CreateEntity();
        ecs.AddComponents(e, DirectionalLight{{0.5f, 0.5f, 0.5f, 1.f}, {0.5f, 0.5f, 0.0f, 1.f}});
        auto mesh = m_allMeshes->meshes.begin()->second;
        register_object(e, mesh, Translation{{3.f, 10.f, 0.f}});
    }

    auto& sceneData = m_renderer.GetGpuSceneData();
    sceneData.ambientColor = glm::vec4(.1f, .1f, .1f, 1.f);
}

HashCubes::~HashCubes()
{
    auto& ecs = Ecs::GetInstance();
    m_deletionQueue.Flush();
    ecs.Destroy();
    AssetMngr::Shutdown();
}

void HashCubes::Run()
{
    auto& ecs = Ecs::GetInstance();
    auto prevTime = std::chrono::high_resolution_clock::now();
    bool running = true;
    SDL_Event event;
    while (running)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - prevTime).count();

        auto keyboardQueue = ecs.GetSingletonComponent<InputQueue<SDL_KeyboardEvent>>();
        auto mouseButtonQueue = ecs.GetSingletonComponent<InputQueue<SDL_MouseButtonEvent>>();
        auto mouseMotionQueue = ecs.GetSingletonComponent<InputQueue<SDL_MouseMotionEvent>>();
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                keyboardQueue->queue.push_back(event.key);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                mouseButtonQueue->queue.push_back(event.button);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                mouseButtonQueue->queue.push_back(event.button);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mouseMotionQueue->queue.push_back(event.motion);
                break;
            default:
                break;
            }

            ImGui_ImplSDL3_ProcessEvent(&event);
        }

        ecs.UpdateSystems(dt);
        m_renderer.WaitIdle();
        prevTime = currentTime;
        std::cout.flush();
    }
}

void HashCubes::initEcs()
{
    auto& ecs = Ecs::GetInstance();

    ecs.AddSystem<InputSystem>(InputSystem(m_window.window()));
    ecs.AddSystem<CameraSystem>(CameraSystem());
    ecs.AddSystem<MovementSystem>(MovementSystem());
    ecs.AddSystem<TransformSystem>(TransformSystem());
    ecs.AddSystem<LightingSystem>(&m_renderer);
    ecs.AddSystem<RenderSystem>(&m_renderer);
    ecs.AddSystem<PerformanceMeasureSystem>(PerformanceMeasureSystem());

    ecs.AddSingletonComponent(FramesPerSecond{});
    ecs.AddSingletonComponent(MouseMode{});

    ecs.AddSingletonComponent(InputQueue<SDL_KeyboardEvent>());
    ecs.AddSingletonComponent(InputQueue<SDL_MouseButtonEvent>());
    ecs.AddSingletonComponent(InputQueue<SDL_MouseMotionEvent>());
}
