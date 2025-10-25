#include "upnp.hpp"
#include "log.hpp"

bool ForwardPort(const char* Port, const char* AppName) {
	bool result = true;
	struct UPNPUrls upnp_urls;
	struct IGDdatas upnp_data;
	struct UPNPDev* upnp_dev = 0;
	char aLanAddr[64];

	int error = 0;
	upnp_dev = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &error);
	// Retrieve a valid Internet Gateway Device
	int status = UPNP_GetValidIGD(upnp_dev, &upnp_urls, &upnp_data, aLanAddr,
								  sizeof(aLanAddr), nullptr, 0);
	LOGI("[UPNP] Status=%d, Lan_addr=%s", status, aLanAddr);

	if (status == 1) {
		error =
			UPNP_AddPortMapping(upnp_urls.controlURL, upnp_data.first.servicetype,
								Port, // external port
								Port, // internal port
								aLanAddr, AppName, "UDP",
								0,  // remote host
								"0" // lease duration, recommended 0 as some NAT
									// implementations may not support another value
			);

		if (error) {
			LOGE("[UPNP] Failed to map port");
			LOGE("[UPNP] Error: %s", strupnperror(error));
			result = false;
		}
		else
			LOGI("[UPNP] Successfully mapped port");
	}
	else {
		LOGE("[UPNP] No valid IGD found\n");
		result = false;
	}

	return result;
}

bool DeletePort(const char* Port) {
	bool result = true;
	struct UPNPUrls upnp_urls;
	struct IGDdatas upnp_data;
	struct UPNPDev* upnp_dev = 0;
	char aLanAddr[64];
	int error = 0;

	upnp_dev = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &error);
	int status = UPNP_GetValidIGD(upnp_dev, &upnp_urls, &upnp_data, aLanAddr, sizeof(aLanAddr), nullptr, 0);
	LOGI("[UPNP] Status=%d, Lan_addr=%s", status, aLanAddr);

	if (status == 1) {
		error = UPNP_DeletePortMapping(upnp_urls.controlURL, upnp_data.first.servicetype,
				   Port, "UDP", 0);

		if (error != 0) {
			LOGE("[UPNP] Port map deletion error: %s\n", strupnperror(error));
			result = false;
		}
	}
	else {
		LOGE("[UPNP] No valid IGD found\n");
		result = false;
	}

	FreeUPNPUrls(&upnp_urls);
	freeUPNPDevlist(upnp_dev);
	return result;
}
