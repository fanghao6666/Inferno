#include "application.h"
#include "vk.h"

#include <memory>

namespace inferno
{
class Runtime : public Application
{
public:
    Runtime()
    {
    }

    ~Runtime()
    {
    }

protected:
    bool init(int argc, const char* argv[]) override
    {
        m_backend = vk::Backend::create(m_window, true);

        return true;
    }

    AppSettings intial_app_settings() override
    {
        AppSettings settings;
        return settings;
    }

private:
    vk::Backend::Ptr m_backend;
};
} // namespace inferno

INFERNO_DECLARE_MAIN(inferno::Runtime);