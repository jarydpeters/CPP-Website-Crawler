#include <iostream>
#include <curl/curl.h>
#include <regex>
#include <set>
#include <queue>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

// Global debug flag
bool debug = false;

// Set of visited URLs
std::set<std::string> visitedUrls;

// Queue for URLs to process
std::queue<std::string> urlQueue;

// Base domain restriction
std::string baseDomain = "https://distek.com";

// List of search keywords
std::vector<std::string> keywords = {};

// Output file for matched URLs
std::ofstream matchedUrlsFile("matched_urls.txt", std::ios::app);

// Callback to store downloaded HTML
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Fetch HTML content using cURL
std::string fetchHTML(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string html;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error fetching " << url << ": " << curl_easy_strerror(res) << std::endl;
        }

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (debug) {
            std::cout << "Response Code for " << url << ": " << response_code << std::endl;
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize cURL for " << url << std::endl;
    }
    return html;
}

// Extract links from HTML
std::vector<std::string> extractLinks(const std::string& html) {
    std::vector<std::string> links;
    std::regex linkRegex(R"(<a\s+(?:[^>]*?\s+)?href=["'](https?://[^"']+)["'])");
    std::smatch match;
    std::string::const_iterator searchStart(html.cbegin());
    while (std::regex_search(searchStart, html.cend(), match, linkRegex)) {
        std::string link = match[1].str();
        if (link.find(baseDomain) != std::string::npos) {
            links.push_back(link);
            if (debug) {
                std::cout << "Found internal link: " << link << std::endl;
            }
        }
        searchStart = match.suffix().first;
    }
    return links;
}

// Perform keyword search on a page
void searchKeywords(const std::string& html, const std::string& url) {
    for (const auto& keyword : keywords) {
        std::string lowerHtml = html;
        std::transform(lowerHtml.begin(), lowerHtml.end(), lowerHtml.begin(), ::tolower);
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

        std::string pattern = R"(\b)" + lowerKeyword + R"(\b)";
        std::regex wordRegex(pattern);

        if (std::regex_search(lowerHtml, wordRegex)) {
            std::cout << "\n[MATCH] Found keyword '" << keyword << "' in " << url << "\n\n";
            matchedUrlsFile << "Found keyword '" << keyword << "' in " << url << "\n";
        }
    }
}

// Check if URL is an image
bool isImageUrl(const std::string& url) {
    const std::vector<std::string> imageExtensions = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".svg", ".webp"
    };
    for (const auto& ext : imageExtensions) {
        if (url.rfind(ext) == url.length() - ext.length()) {
            return true;
        }
    }
    return false;
}

// Core crawler logic
void crawl(const std::string& startUrl, bool doSearch) {
    urlQueue.push(startUrl);
    visitedUrls.insert(startUrl);

    while (!urlQueue.empty()) {
        std::string currentUrl = urlQueue.front();
        urlQueue.pop();

        std::cout << "Visiting: " << currentUrl << "\n";
        std::cout << "URL Queue Size: " << urlQueue.size() << "\n";

        std::string html = fetchHTML(currentUrl);
        if (doSearch) {
            searchKeywords(html, currentUrl);
        }

        std::vector<std::string> links = extractLinks(html);
        for (const auto& link : links) {
            if (visitedUrls.find(link) == visitedUrls.end() && !isImageUrl(link)) {
                visitedUrls.insert(link);
                urlQueue.push(link);
                if (debug) {
                    std::cout << "Queueing link: " << link << "\n";
                }
            } else {
                if (debug) {
                    std::cout << "Already visited: " << link << "\n";
                }
            }
        }
    }

    std::cout << "Crawling complete.\n";
}

// Print help
void printUsage(const std::string& programName) {
    std::cout << "Usage:\n"
              << "  " << programName << " --search \"phrase\"       Search for a keyword\n"
              << "  " << programName << " --sitemap                 Generate a sitemap\n"
              << "Optional:\n"
              << "  --start-url \"https://example.com\"            Custom start URL\n";
}

// Entry point
int main(int argc, char* argv[]) {
    std::string startUrl = "https://distek.com";
    bool doSearch = false;
    bool doSitemap = false;
    std::string searchPhrase;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--search" && i + 1 < argc) {
            doSearch = true;
            searchPhrase = argv[++i];
            keywords = { searchPhrase };
        } else if (arg == "--sitemap") {
            doSitemap = true;
        } else if (arg == "--start-url" && i + 1 < argc) {
            startUrl = argv[++i];
            baseDomain = startUrl.substr(0, startUrl.find("/", 9)); // adjust domain for custom URLs
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    if (!doSearch && !doSitemap) {
        printUsage(argv[0]);
        return 1;
    }

    crawl(startUrl, doSearch);

    if (doSitemap) {
        std::ofstream sitemapFile("sitemap.txt");
        for (const auto& url : visitedUrls) {
            sitemapFile << url << "\n";
        }
        sitemapFile.close();
        std::cout << "\n✅ Sitemap saved to sitemap.txt\n";
    }

    matchedUrlsFile.close();
    return 0;
}
