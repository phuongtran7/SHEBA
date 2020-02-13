#include <fmt/format.h>
#include <cpprest/http_client.h>
#include "rapidjson/document.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace http;                  // Common HTTP functionality
using namespace client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams


int main()
{
	struct RepoInfo {
		std::string name;
		int clones;
		int uniqueClones;
		int views;
		int UniqueViews;
	};

	std::vector<RepoInfo> Infos;
	web::http::client::http_client client_(U("https://api.github.com/"));
	
	std::string user = "phuongtran7";

	// Build request URI and start the request.
	web::uri_builder builder;
	builder.set_path(utility::conversions::to_string_t(fmt::format("/users/{}/repos", user)));

	pplx::task<std::vector<std::string>> request_task = client_.request(methods::GET, builder.to_string())
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					fmt::print("Received response status code from Custom Field querry: {}.", response.status_code());
					throw;
				}

				return response.extract_utf8string();
			})
		.then([=](const std::string& json_data)
			{
				rapidjson::Document document;
				document.Parse(json_data.c_str());

				std::vector<std::string> repos;

				// Only get the repo that is not a fork
				for (const auto& object : document.GetArray()) {
					if (!object.FindMember("fork")->value.GetBool())
					{
						repos.emplace_back(object.FindMember("name")->value.GetString());
					}
				}
				return repos;
			});

			try
			{
				request_task.wait();
			}
			catch (const std::exception & e)
			{
				fmt::print("Error exception: {}", e.what());
			}
}
