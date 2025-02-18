#include <iostream>
#include <curl/curl.h>
#include <regex>
#include <set>
#include <queue>
#include <string>
#include <vector>
#include <fstream>  // For file writing
#include <algorithm>

// Global debug flag
bool debug = false;  // Set to false to suppress debug output

// Set of visited URLs
std::set<std::string> visitedUrls;

// Queue for URLs to process
std::queue<std::string> urlQueue;

// Base domain restriction
std::string baseDomain = "https://distek.com";

// List of search keywords
std::vector<std::string> keywords = {"VT Anywhere"};

// Output file for matched URLs
std::ofstream matchedUrlsFile("matched_urls.txt", std::ios::app);  // Open file for appending

// Callback function to store downloaded HTML
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) 
{
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to download webpage
std::string fetchHTML(const std::string& url) 
{
    CURL* curl;
    CURLcode res;
    std::string html;

    curl = curl_easy_init();
    if (curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");  // Set a user agent to avoid being blocked
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Disable SSL verification
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);  // Disable SSL verification

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) 
        {
            std::cerr << "Error while fetching " << url << ": " << curl_easy_strerror(res) << std::endl;
        }

        // Get and print the response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (debug) 
        {
            std::cout << "Response Code for " << url << ": " << response_code << std::endl;
        }

        curl_easy_cleanup(curl);
    } 
    else 
    {
        std::cerr << "Failed to initialize cURL for " << url << std::endl;
    }
    return html;
}

// Extract links from HTML using regex
std::vector<std::string> extractLinks(const std::string& html) 
{
    std::vector<std::string> links;
    std::regex linkRegex(R"(<a\s+(?:[^>]*?\s+)?href=["'](https?://[^"']+)["'])");
    std::smatch match;

    std::string::const_iterator searchStart(html.cbegin());
    while (std::regex_search(searchStart, html.cend(), match, linkRegex)) 
    {
        std::string link = match[1].str();
        if (link.find(baseDomain) != std::string::npos) 
        {
            links.push_back(link);
            if (debug) 
            {
                std::cout << "Found internal link: " << link << std::endl;
            }
        }
        searchStart = match.suffix().first;
    }
    return links;
}

// Function to perform case-insensitive search for whole words only
void searchKeywords(const std::string& html, const std::string& url) 
{
    for (const auto& keyword : keywords) 
    {
        // Create a case-insensitive regex for whole words only
        std::string lowerHtml = html;
        std::transform(lowerHtml.begin(), lowerHtml.end(), lowerHtml.begin(), ::tolower);
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

        // Create regex pattern for matching the whole word
        std::string pattern = R"(\b)" + lowerKeyword + R"(\b)";
        std::regex wordRegex(pattern);

        // Perform the search using the regex
        if (std::regex_search(lowerHtml, wordRegex)) 
        {
            std::cout << "\n[MATCH] Found keyword '" << keyword << "' in " << url << "\n\n";

            // Write the matched URL to the file
            matchedUrlsFile << "Found keyword '" << keyword << "' in " << url << "\n";
        }
    }
}

// Crawler function
void crawl(const std::string& startUrl) 
{
    urlQueue.push(startUrl);
    visitedUrls.insert(startUrl);  // Mark the starting URL as visited right away

    while (!urlQueue.empty()) 
    {
        std::string currentUrl = urlQueue.front();
        urlQueue.pop();

        std::cout << "Visiting: " << currentUrl << "\n";
        std::cout << "URL Queue Size: " << urlQueue.size() << " URLs remaining in the queue." << std::endl;

        std::string html = fetchHTML(currentUrl);
        searchKeywords(html, currentUrl);

        std::vector<std::string> links = extractLinks(html);
        for (const auto& link : links) 
        {
            if (visitedUrls.find(link) == visitedUrls.end()) 
            {
                visitedUrls.insert(link);  // Mark this link as visited
                urlQueue.push(link);  // Queue the link for processing
                if (debug) 
                {
                    std::cout << "Queueing link: " << link << "\n";  // For debug visibility
                }
            } 
            else 
            {
                if (debug) 
                {
                    std::cout << "Already visited: " << link << "\n";  // For debug visibility
                }
            }
        }
    }

    std::cout << "Crawling complete! All pages have been visited." << std::endl;
}

int main() 
{
    std::string startUrl = "https://distek.com";
    crawl(startUrl);

    // Close the file after all processing is complete
    matchedUrlsFile.close();
    return 0;
}
