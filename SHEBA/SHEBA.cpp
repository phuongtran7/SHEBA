#include <fmt/format.h>
#include <cpprest/http_client.h>
#include "rapidjson/document.h"
#include <tabulate/table.hpp>
#include <concurrent_unordered_map.h>
#include "toml.hpp"

using namespace utility;
using namespace web;
using namespace http;
using namespace client;
using namespace concurrency::streams;

struct RepoInfo {
	std::string name;
	int clones;
	int uniqueClones;
	int views;
	int UniqueViews;
};

std::vector<std::string> GetAllPublicRepo(web::http::client::http_client& client, const std::string& user) {
	// Build request URI and start the request.
	web::uri_builder builder;
	builder.set_path(utility::conversions::to_string_t(fmt::format("/users/{}/repos", user)));

	pplx::task<std::vector<std::string>> request_task = client.request(methods::GET, builder.to_string())
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					fmt::print("Received response status code from get public repo querry: {}.", response.status_code());
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
		return request_task.get();
	}
	catch (const std::exception & e)
	{
		fmt::print("Error: {}\n", e.what());
	}
}

std::vector<RepoInfo> BuildDatabase(web::http::client::http_client& client, const std::string& user, const std::string& token, const std::vector<std::string>& input) {

	struct count {
		int count;
		int unique;
	};

	Concurrency::concurrent_unordered_map<std::string, count> views;
	Concurrency::concurrent_unordered_map<std::string, count> clones;

	std::vector<pplx::task<void>> result;

	for (auto& repo : input) {
		web::uri_builder view_builder;
		view_builder.set_path(utility::conversions::to_string_t(fmt::format("/repos/{}/{}/traffic/views", user, repo)));

		http_request view_request(methods::GET);
		auto formated = fmt::format("Token {}", token);
		view_request.headers().add(L"Authorization", conversions::to_string_t(formated));
		view_request.set_request_uri(view_builder.to_string());

		pplx::task<void>request_view_task = client.request(view_request).then([&](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					fmt::print("Received response status code from get view querry: {}.\n", response.status_code());
					throw;
				}

				return response.extract_utf8string();
			})
			.then([&](const std::string& json_data)
				{
					rapidjson::Document document;
					document.Parse(json_data.c_str());
					count temp{ document["count"].GetInt(), document["uniques"].GetInt() };
					views.insert(std::make_pair(repo, temp));
				});

		result.push_back(std::move(request_view_task));


		web::uri_builder clone_builder;
		clone_builder.set_path(utility::conversions::to_string_t(fmt::format("/repos/{}/{}/traffic/clones", user, repo)));

		http_request clone_request(methods::GET);
		clone_request.headers().add(L"Authorization", conversions::to_string_t(formated));
		clone_request.set_request_uri(clone_builder.to_string());

		pplx::task<void>request_clone_task = client.request(clone_request).then([&](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					fmt::print("Received response status code from get clone querry: {}.\n", response.status_code());
					throw;
				}

				return response.extract_utf8string();
			})
			.then([&](const std::string& json_data)
				{
					rapidjson::Document document;
					document.Parse(json_data.c_str());
					count temp{ document["count"].GetInt(), document["uniques"].GetInt() };
					clones.insert(std::make_pair(repo, temp));
				});

		result.push_back(std::move(request_clone_task));
	}

	for (auto& task : result) {
		try
		{
			task.wait();
		}
		catch (const std::exception & e)
		{
			fmt::print("Error: {}\n", e.what());
		}
	}

	std::vector<RepoInfo> return_val{};
	for (auto& repo : input) {
		auto view = views.find(repo)->second;
		auto clone = clones.find(repo)->second;
		return_val.emplace_back(RepoInfo{repo, view.count, view.unique, clone.count, clone.unique});
	}

	return return_val;
}

void DrawTable(const std::vector<RepoInfo>& input) {
	using namespace tabulate;
	Table data;
	data.add_row({ "Name", "Views", "Unique Views", "Clones", "Unique Clones" });

	for (auto& repo : input) {
		data.add_row({ repo.name, std::to_string(repo.views), std::to_string(repo.UniqueViews), std::to_string(repo.clones), std::to_string(repo.uniqueClones) });
	}

	// Center-align and color header cells
	for (auto i = 0; i < 5; ++i) {
		data[0][i].format()
			.font_color(Color::cyan)
			.font_align(FontAlign::center)
			.font_style({ FontStyle::bold });
	}

	// Center-align all collumns
	for (auto i = 0; i < 5; ++i) {
		data.column(i).format().font_align(FontAlign::center);
	}

	std::cout << data << std::endl;
}

int main()
{
	const auto data = toml::parse("Secrets.toml");
	std::string user = toml::find<std::string>(data, "User");
	std::string token = toml::find<std::string>(data, "Token");

	web::http::client::http_client client_(U("https://api.github.com/"));

	auto repos = GetAllPublicRepo(client_, user);

	auto database = BuildDatabase(client_, user, token, repos);

	DrawTable(database);

	std::getchar();
}
