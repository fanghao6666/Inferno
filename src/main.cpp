#include "application.h"
#include "vk.h"

#include <memory>

namespace inferno
{
const std::vector<const char*> g_validation_layers = 
{
    "VK_LAYER_LUNARG_standard_validation"
};

class InfernoApp : public Application
{
public:
    InfernoApp()
    {
        m_instance = std::make_unique<vk::Instance>("Inferno", g_validation_layers);
    }

    ~InfernoApp()
    {
    }

protected:
    inferno::AppSettings intial_app_settings() override
    {
        inferno::AppSettings settings;
        return settings;
    }

private:
    std::unique_ptr<vk::Instance> m_instance;
};
}

INFERNO_DECLARE_MAIN(inferno::InfernoApp);