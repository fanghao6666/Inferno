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
		m_backend = std::make_unique<vk::Backend>(m_window, true);
		
		return true;
	}

    AppSettings intial_app_settings() override
    {
        AppSettings settings;
        return settings;
    }

private:
    std::unique_ptr<vk::Backend> m_backend;
};
}

INFERNO_DECLARE_MAIN(inferno::Runtime);