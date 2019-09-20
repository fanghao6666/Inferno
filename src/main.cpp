#include "application.h"

class InfernoApp : public inferno::Application
{
public:
    InfernoApp()
	{

	}

	~InfernoApp()
	{

	}

protected:
	// Intial app settings. Override this to set defaults.
	inferno::AppSettings intial_app_settings() override
	{
		inferno::AppSettings settings;
		return settings;
	}
};

INFERNO_DECLARE_MAIN(InfernoApp);