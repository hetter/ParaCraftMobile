#include "ParaEngine.h"
#include "Downloader.h"
#include "AssetsManagerGlobal.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <cstdio>
#include <cerrno>
#include <thread>
namespace AutoUpdate
{
#define LOW_SPEED_LIMIT     1L
#define LOW_SPEED_TIME      5L
#define MAX_REDIRS          2
#define DEFAULT_TIMEOUT     5
#define HTTP_CODE_SUPPORT_RESUME    206
#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */

#define TEMP_EXT            ".temp"
#define FOPEN_MAX			1
size_t fileWriteFunc(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    FILE *fp = (FILE*)userdata;
    size_t written = fwrite(ptr, size, nmemb, fp);
    return written;
}

size_t bufferWriteFunc(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    Downloader::StreamData *streamBuffer = (Downloader::StreamData *)userdata;
    size_t written = size * nmemb;
    // Avoid pointer overflow
	if (streamBuffer->offset + written <= static_cast<size_t>(streamBuffer->total))
    {
        memcpy(streamBuffer->buffer + streamBuffer->offset, ptr, written);
        streamBuffer->offset += written;
        return written;
    }
    else return 0;
}

// This is only for batchDownload process, will notify file succeed event in progress function
int batchDownloadProgressFunc(Downloader::ProgressData *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
{
	Downloader::ProgressData data = *ptr;
	totalToDownload += data.startToDownload;
	nowDownloaded += data.startToDownload;
    if (ptr->totalToDownload == 0)
    {
        ptr->totalToDownload = totalToDownload;
    }
    
    if (ptr->downloaded != nowDownloaded)
    {
        ptr->downloaded = nowDownloaded;
       
        if (nowDownloaded == totalToDownload)
        {
            AssetsManagerGlobal::performFunctionInCocosThread([=]{
                if (!data.downloader.expired())
                {
                    std::shared_ptr<Downloader> downloader = data.downloader.lock();
                
                    auto progressCB = downloader->getProgressCallback();
                    if (progressCB != nullptr)
                    {
                        progressCB(totalToDownload, nowDownloaded, data.url, data.customId);
                    }
                    auto successCB = downloader->getSuccessCallback();
                    if (successCB != nullptr)
                    {
                        successCB(data.url, data.path + data.name, data.customId);
                    }
                }
            });
        }
        else
        {
            AssetsManagerGlobal::performFunctionInCocosThread([=]{
                if (!data.downloader.expired())
                {
                    std::shared_ptr<Downloader> downloader = data.downloader.lock();
                
                    auto callback = downloader->getProgressCallback();
                    if (callback != nullptr)
                    {
                        callback(totalToDownload, nowDownloaded, data.url, data.customId);
                    }
                }
            });
        }
    }
    
    return 0;
}

// Compare to batchDownloadProgressFunc, this only handles progress information notification
int downloadProgressFunc(Downloader::ProgressData *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
{
    if (ptr->totalToDownload == 0)
    {
        ptr->totalToDownload = totalToDownload;
    }
    
    if (ptr->downloaded != nowDownloaded)
    {
        ptr->downloaded = nowDownloaded;
        Downloader::ProgressData data = *ptr;
        
        AssetsManagerGlobal::performFunctionInCocosThread([=]{
            if (!data.downloader.expired())
            {
                std::shared_ptr<Downloader> downloader = data.downloader.lock();
                
                auto callback = downloader->getProgressCallback();
                if (callback != nullptr)
                {
                    callback(totalToDownload, nowDownloaded, data.url, data.customId);
                }
            }
        });
    }
    
    return 0;
}

Downloader::Downloader()
: _connectionTimeout(DEFAULT_TIMEOUT)
, _onError(nullptr)
, _onProgress(nullptr)
, _onSuccess(nullptr)
, _supportResuming(false)
{
}

Downloader::~Downloader()
{
}

int Downloader::getConnectionTimeout()
{
    return _connectionTimeout;
}

void Downloader::setConnectionTimeout(int timeout)
{
    if (timeout >= 0)
        _connectionTimeout = timeout;
}

void Downloader::notifyError(ErrorCode code, const std::string &msg/* ="" */, const std::string &customId/* ="" */, int curle_code/* = CURLE_OK*/, int curlm_code/* = CURLM_OK*/)
{
    std::weak_ptr<Downloader> ptr = shared_from_this();
    AssetsManagerGlobal::performFunctionInCocosThread([=]{
        if (!ptr.expired())
        {
            std::shared_ptr<Downloader> downloader = ptr.lock();
            if (downloader->_onError != nullptr)
            {
                Error err;
                err.code = code;
                err.curle_code = curle_code;
                err.curlm_code = curlm_code;
                err.message = msg;
                err.customId = customId;
                downloader->_onError(err);
            }
        }
    });
}

void Downloader::notifyError(const std::string &msg, int curlm_code, const std::string &customId/* = ""*/)
{
    notifyError(ErrorCode::CURL_MULTI_ERROR, msg, customId, CURLE_OK, curlm_code);
}

void Downloader::notifyError(const std::string &msg, const std::string &customId, int curle_code)
{
    notifyError(ErrorCode::CURL_EASY_ERROR, msg, customId, curle_code);
}

std::string Downloader::getFileNameFromUrl(const std::string &srcUrl)
{
    // Find file name and file extension
    std::string filename;
    unsigned long found = srcUrl.find_last_of("/\\");
    if (found != std::string::npos)
        filename = srcUrl.substr(found+1);
    return filename;
}

void Downloader::clearBatchDownloadData()
{
    while (_progDatas.size() != 0) {
        delete _progDatas.back();
        _progDatas.pop_back();
    }
    
    while (_files.size() != 0) {
        delete _files.back();
        _files.pop_back();
    }
}

void Downloader::prepareDownload(const std::string &srcUrl, const std::string &storagePath, const std::string &customId, bool resumeDownload, FileDescriptor *fDesc, ProgressData *pData)
{
    std::shared_ptr<Downloader> downloader = shared_from_this();
    pData->customId = customId;
    pData->url = srcUrl;
    pData->downloader = downloader;
    pData->downloaded = 0;
    pData->totalToDownload = 0;
    
    fDesc->fp = nullptr;
    fDesc->curl = nullptr;
    
    Error err;
    err.customId = customId;
    
    // Asserts
    // Find file name and file extension
    unsigned long found = storagePath.find_last_of("/\\");
    if (found != std::string::npos)
    {
        pData->name = storagePath.substr(found+1);
        pData->path = storagePath.substr(0, found+1);
    }
    else
    {
        err.code = ErrorCode::INVALID_URL;
        err.message = "Invalid url or filename not exist error: " + srcUrl;
        if (this->_onError) this->_onError(err);
        return;
    }
    
    // Create a file to save file.
    const std::string outFileName = storagePath + TEMP_EXT;
    if (_supportResuming && resumeDownload && FileUtils::isFileExist(outFileName))
    {
        fDesc->fp = fopen(outFileName.c_str(), "ab");
		long size = FileUtils::getFileSize(outFileName);
		pData->startToDownload = size;
		OUTPUT_LOG1("prepare to resume download:%d %s\n", size,srcUrl.c_str());
    }
    else
    {
        fDesc->fp = fopen(outFileName.c_str(), "wb");
		pData->startToDownload = 0;
		OUTPUT_LOG1("prepare to download:%s\n", srcUrl.c_str());
	}
    if (!fDesc->fp)
    {
        err.code = ErrorCode::CREATE_FILE;
        err.message = AssetsManagerGlobal::format("Can not create file %s: errno %d", outFileName.c_str(), errno);
        if (this->_onError) this->_onError(err);
    }
}

bool Downloader::prepareHeader(void *curl, const std::string &srcUrl) const
{
    curl_easy_setopt(curl, CURLOPT_URL, srcUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    if (curl_easy_perform(curl) == CURLE_OK)
        return true;
    else
        return false;
}

long Downloader::getContentSize(const std::string &srcUrl) const
{
    double contentLength = -1;
    CURL *header = curl_easy_init();
    if (prepareHeader(header, srcUrl))
    {
        curl_easy_getinfo(header, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
    }
    curl_easy_cleanup(header);
    
    return contentLength;
}

void Downloader::downloadToBufferAsync(const std::string &srcUrl, unsigned char *buffer, const long &size, const std::string &customId/* = ""*/)
{
    if (buffer != nullptr)
    {
        std::shared_ptr<Downloader> downloader = shared_from_this();
        ProgressData pData;
        pData.customId = customId;
        pData.url = srcUrl;
        pData.downloader = downloader;
        pData.downloaded = 0;
        pData.totalToDownload = 0;
        
        StreamData streamBuffer;
        streamBuffer.buffer = buffer;
        streamBuffer.total = size;
        streamBuffer.offset = 0;
        
        auto t = std::thread(&Downloader::downloadToBuffer, this, srcUrl, customId, streamBuffer, pData);
        t.detach();
    }
}

void Downloader::downloadToBufferSync(const std::string &srcUrl, unsigned char *buffer, const long &size, const std::string &customId/* = ""*/)
{
    if (buffer != nullptr)
    {
        std::shared_ptr<Downloader> downloader = shared_from_this();
        ProgressData pData;
        pData.customId = customId;
        pData.url = srcUrl;
        pData.downloader = downloader;
        pData.downloaded = 0;
        pData.totalToDownload = 0;
        
        StreamData streamBuffer;
        streamBuffer.buffer = buffer;
        streamBuffer.total = size;
        streamBuffer.offset = 0;
        
        downloadToBuffer(srcUrl, customId, streamBuffer, pData);
    }
}

void Downloader::downloadToBuffer(const std::string &srcUrl, const std::string &customId, const StreamData &buffer, const ProgressData &data)
{
    std::weak_ptr<Downloader> ptr = shared_from_this();
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        this->notifyError(ErrorCode::CURL_EASY_ERROR, "Can not init curl with curl_easy_init", customId);
        return;
    }
    
    // Download pacakge
    curl_easy_setopt(curl, CURLOPT_URL, srcUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bufferWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, downloadProgressFunc);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &data);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    if (_connectionTimeout) curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connectionTimeout);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        FileUtils::removeFile(data.path + data.name + TEMP_EXT);
        std::string msg = AssetsManagerGlobal::format("Unable to download file: [curl error]%s", curl_easy_strerror(res));
        this->notifyError(msg, customId, res);
    }
    
    curl_easy_cleanup(curl);
    
    AssetsManagerGlobal::performFunctionInCocosThread([=]{
        if (!ptr.expired())
        {
            std::shared_ptr<Downloader> downloader = ptr.lock();
            
            auto successCB = downloader->getSuccessCallback();
            if (successCB != nullptr)
            {
                successCB(data.url, "", data.customId);
            }
        }
    });
}

void Downloader::downloadAsync(const std::string &srcUrl, const std::string &storagePath, const std::string &customId/* = ""*/)
{
    FileDescriptor fDesc;
    ProgressData pData;
    prepareDownload(srcUrl, storagePath, customId, false, &fDesc, &pData);
    if (fDesc.fp != NULL)
    {
        auto t = std::thread(&Downloader::download, this, srcUrl, customId, fDesc, pData);
        t.detach();
    }
}

void Downloader::downloadSync(const std::string &srcUrl, const std::string &storagePath, const std::string &customId/* = ""*/)
{
    FileDescriptor fDesc;
    ProgressData pData;
    prepareDownload(srcUrl, storagePath, customId, false, &fDesc, &pData);
    if (fDesc.fp != NULL)
    {
        download(srcUrl, customId, fDesc, pData);
    }
}

void Downloader::download(const std::string &srcUrl, const std::string &customId, const FileDescriptor &fDesc, const ProgressData &data)
{
    std::weak_ptr<Downloader> ptr = shared_from_this();
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        this->notifyError(ErrorCode::CURL_EASY_ERROR, "Can not init curl with curl_easy_init", customId);
        return;
    }
    
    // Download pacakge
    curl_easy_setopt(curl, CURLOPT_URL, srcUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fileWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fDesc.fp);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, downloadProgressFunc);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &data);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    if (_connectionTimeout) curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connectionTimeout);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        FileUtils::removeFile(data.path + data.name + TEMP_EXT);
        std::string msg = AssetsManagerGlobal::format("Unable to download file: [curl error]%s", curl_easy_strerror(res));
        this->notifyError(msg, customId, res);
    }
    
    fclose(fDesc.fp);
    curl_easy_cleanup(curl);
    
    // This can only be done after fclose
    if (res == CURLE_OK)
    {
        FileUtils::renameFile(data.path, data.name + TEMP_EXT, data.name);
        
        AssetsManagerGlobal::performFunctionInCocosThread([=]{
            if (!ptr.expired())
            {
                std::shared_ptr<Downloader> downloader = ptr.lock();
                
                auto successCB = downloader->getSuccessCallback();
                if (successCB != nullptr)
                {
                    successCB(data.url, data.path + data.name, data.customId);
                }
            }
        });
    }
}

void Downloader::batchDownloadAsync(const DownloadUnits &units, const std::string &batchId/* = ""*/)
{
    auto t = std::thread(&Downloader::batchDownloadSync, this, units, batchId);
    t.detach();
}

void Downloader::batchDownloadSync(const DownloadUnits &units, const std::string &batchId/* = ""*/)
{
    // Make sure downloader won't be released
    std::weak_ptr<Downloader> ptr = shared_from_this();
    
    if (units.size() != 0)
    {
        // Test server download resuming support with the first unit
        _supportResuming = false;
        CURL *header = curl_easy_init();
        // Make a resume request
		curl_easy_setopt(header, CURLOPT_RANGE, "0-1");
		curl_easy_setopt(header, CURLOPT_URL, units.begin()->second.srcUrl.c_str());
		CURLcode code = curl_easy_perform(header);
		if (code == CURLE_OK)
		{
			long responseCode;
			curl_easy_getinfo(header, CURLINFO_RESPONSE_CODE, &responseCode);
			OUTPUT_LOG1("check server download resuming responseCode:%d %s\n", responseCode, units.begin()->second.srcUrl.c_str());
			if (responseCode == HTTP_CODE_SUPPORT_RESUME)
			{
				_supportResuming = true;
			}
		}
		OUTPUT_LOG1("check server download resuming support:%d\n", _supportResuming);

        curl_easy_cleanup(header);
        
        int count = 0;
        DownloadUnits group;
        for (auto it = units.cbegin(); it != units.cend(); ++it, ++count)
        {
            if (count == FOPEN_MAX)
            {
                groupBatchDownload(group);
                group.clear();
                count = 0;
            }
            const std::string &key = it->first;
            const DownloadUnit &unit = it->second;
            group.emplace(key, unit);
        }
        if (group.size() > 0)
        {
            groupBatchDownload(group);
        }
    }
    
    AssetsManagerGlobal::performFunctionInCocosThread([ptr, batchId]{
        if (!ptr.expired()) {
            std::shared_ptr<Downloader> downloader = ptr.lock();
            auto callback = downloader->getSuccessCallback();
            if (callback != nullptr)
            {
                callback("", "", batchId);
            }
        }
    });
    _supportResuming = false;
}

void Downloader::groupBatchDownload(const DownloadUnits &units)
{
    CURLM* multi_handle = curl_multi_init();
    int still_running = 0;
    
    for (auto it = units.cbegin(); it != units.cend(); ++it)
    {
        DownloadUnit unit = it->second;
        std::string srcUrl = unit.srcUrl;
        std::string storagePath = unit.storagePath;
        std::string customId = unit.customId;
        
        FileDescriptor *fDesc = new FileDescriptor();
        ProgressData *data = new ProgressData();
        prepareDownload(srcUrl, storagePath, customId, unit.resumeDownload, fDesc, data);
        
        if (fDesc->fp != NULL)
        {
            CURL* curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, srcUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fileWriteFunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fDesc->fp);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, batchDownloadProgressFunc);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, data);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
            if (_connectionTimeout) curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connectionTimeout);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
            curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, MAX_REDIRS);
            
            // Resuming download support
            if (_supportResuming && unit.resumeDownload)
            {
                // Check already downloaded size for current download unit
                long size = FileUtils::getFileSize(storagePath + TEMP_EXT);
                if (size != -1)
                {
                    curl_easy_setopt(curl, CURLOPT_RESUME_FROM, size);
                }
            }
            fDesc->curl = curl;
            
            CURLMcode code = curl_multi_add_handle(multi_handle, curl);
            if (code != CURLM_OK)
            {
                // Avoid memory leak
                fclose(fDesc->fp);
                delete data;
                delete fDesc;
                std::string msg = AssetsManagerGlobal::format("Unable to add curl handler for %s: [curl error]%s", customId.c_str(), curl_multi_strerror(code));
                this->notifyError(msg, code, customId);
            }
            else
            {
                // Add to list for tracking
                _progDatas.push_back(data);
                _files.push_back(fDesc);
            }
        }
    }
    
    // Query multi perform
    CURLMcode curlm_code = CURLM_CALL_MULTI_PERFORM;
    while(CURLM_CALL_MULTI_PERFORM == curlm_code) {
        curlm_code = curl_multi_perform(multi_handle, &still_running);
    }
    if (curlm_code != CURLM_OK) {
        std::string msg = AssetsManagerGlobal::format("Unable to continue the download process: [curl error]%s", curl_multi_strerror(curlm_code));
        this->notifyError(msg, curlm_code);
    }
    else
    {
        bool failed = false;
        while (still_running > 0 && !failed)
        {
            // set a suitable timeout to play around with
            struct timeval select_tv;
            long curl_timeo = -1;
            select_tv.tv_sec = 1;
            select_tv.tv_usec = 0;
            
            curl_multi_timeout(multi_handle, &curl_timeo);
            if(curl_timeo >= 0) {
                select_tv.tv_sec = curl_timeo / 1000;
                if(select_tv.tv_sec > 1)
                    select_tv.tv_sec = 1;
                else
                    select_tv.tv_usec = (curl_timeo % 1000) * 1000;
            }
            
            int rc;
            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = -1;
            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);
// FIXME: when jenkins migrate to ubuntu, we should remove this hack code
#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
            curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
            rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &select_tv);
#else          
            rc = curl_multi_wait(multi_handle,nullptr, 0, MAX_WAIT_MSECS, &maxfd);
#endif            
            
            switch(rc)
            {
                case -1:
                    failed = true;
                    break;
                case 0:
                default:
                    curlm_code = CURLM_CALL_MULTI_PERFORM;
                    while(CURLM_CALL_MULTI_PERFORM == curlm_code) {
                        curlm_code = curl_multi_perform(multi_handle, &still_running);
                    }
                    if (curlm_code != CURLM_OK) {
                        std::string msg = AssetsManagerGlobal::format("Unable to continue the download process: [curl error]%s", curl_multi_strerror(curlm_code));
                        this->notifyError(msg, curlm_code);
                    }
                    break;
            }
        }
    }
    
    // Clean up and close files
    curl_multi_cleanup(multi_handle);
    for (auto it = _files.begin(); it != _files.end(); ++it)
    {
        FILE *f = (*it)->fp;
        fclose(f);
        auto single = (*it)->curl;
        curl_multi_remove_handle(multi_handle, single);
        curl_easy_cleanup(single);
    }
    
    // Check unfinished files and notify errors, succeed files will be renamed from temporary file name to real name
    for (auto it = _progDatas.begin(); it != _progDatas.end(); ++it) {
        ProgressData *data = *it;
        if (data->downloaded < data->totalToDownload || data->totalToDownload == 0)
        {
            this->notifyError(ErrorCode::NETWORK, "Unable to download file", data->customId);
        }
        else
        {
            FileUtils::renameFile(data->path, data->name + TEMP_EXT, data->name);
        }
    }
    
    clearBatchDownloadData();
}

}
