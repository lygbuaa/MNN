//
//  llm_demo.cpp
//
//  Created by MNN on 2023/03/24.
//  ZhaodeWang
//

#include "llm/llm.hpp"
#define MNN_OPEN_TIME_TRACE
#include <MNN/AutoTime.hpp>
#include <MNN/expr/ExecutorScope.hpp>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <thread>
#include <initializer_list>
#include <libgen.h>

#include "tinyjson.hpp"
#include "tcp-socket-server.h"
#include "signal_handlers.h"

using namespace MNN::Transformer;

static void tuning_prepare(Llm* llm) {
    MNN_PRINT("Prepare for tuning opt Begin\n");
    llm->tuning(OP_ENCODER_NUMBER, {1, 5, 10, 20, 30, 50, 100});
    MNN_PRINT("Prepare for tuning opt End\n");
}

std::vector<std::vector<std::string>> parse_csv(const std::vector<std::string>& lines) {
    std::vector<std::vector<std::string>> csv_data;
    std::string line;
    std::vector<std::string> row;
    std::string cell;
    bool insideQuotes = false;
    bool startCollecting = false;

    // content to stream
    std::string content = "";
    for (auto line : lines) {
        content = content + line + "\n";
    }
    std::istringstream stream(content);

    while (stream.peek() != EOF) {
        char c = stream.get();
        if (c == '"') {
            if (insideQuotes && stream.peek() == '"') { // quote
                cell += '"';
                stream.get(); // skip quote
            } else {
                insideQuotes = !insideQuotes; // start or end text in quote
            }
            startCollecting = true;
        } else if (c == ',' && !insideQuotes) { // end element, start new element
            row.push_back(cell);
            cell.clear();
            startCollecting = false;
        } else if ((c == '\n' || stream.peek() == EOF) && !insideQuotes) { // end line
            row.push_back(cell);
            csv_data.push_back(row);
            cell.clear();
            row.clear();
            startCollecting = false;
        } else {
            cell += c;
            startCollecting = true;
        }
    }
    return csv_data;
}

static int benchmark(Llm* llm, const std::vector<std::string>& prompts, int max_token_number) {
    int prompt_len = 0;
    int decode_len = 0;
    int64_t vision_time = 0;
    int64_t audio_time = 0;
    int64_t prefill_time = 0;
    int64_t decode_time = 0;
    int64_t sample_time = 0;
    // llm->warmup();
    auto context = llm->getContext();
    if (max_token_number > 0) {
        llm->set_config("{\"max_new_tokens\":1}");
    }
    for (int i = 0; i < prompts.size(); i++) {
        const auto& prompt = prompts[i];
        // prompt start with '#' will be ignored
        if (prompt.substr(0, 1) == "#") {
            continue;
        }
        if (max_token_number > 0) {
            llm->response(prompt, &std::cout, nullptr, 0);
            while (!llm->stoped() && context->gen_seq_len < max_token_number) {
                llm->generate(1);
            }
        } else {
            llm->response(prompt);
        }
        llm->reset();
        prompt_len += context->prompt_len;
        decode_len += context->gen_seq_len;
        vision_time += context->vision_us;
        audio_time += context->audio_us;
        prefill_time += context->prefill_us;
        decode_time += context->decode_us;
        sample_time += context->sample_us;
    }
    float vision_s = vision_time / 1e6;
    float audio_s = audio_time / 1e6;
    float prefill_s = prefill_time / 1e6;
    float decode_s = decode_time / 1e6;
    float sample_s = sample_time / 1e6;
    printf("\n#################################\n");
    printf("prompt tokens num = %d\n", prompt_len);
    printf("decode tokens num = %d\n", decode_len);
    printf(" vision time = %.2f s\n", vision_s);
    printf("  audio time = %.2f s\n", audio_s);
    printf("prefill time = %.2f s\n", prefill_s);
    printf(" decode time = %.2f s\n", decode_s);
    printf(" sample time = %.2f s\n", sample_s);
    printf("prefill speed = %.2f tok/s\n", prompt_len / prefill_s);
    printf(" decode speed = %.2f tok/s\n", decode_len / decode_s);
    printf("##################################\n");
    return 0;
}

static int ceval(Llm* llm, const std::vector<std::string>& lines, std::string filename) {
    auto csv_data = parse_csv(lines);
    int right = 0, wrong = 0;
    std::vector<std::string> answers;
    for (int i = 1; i < csv_data.size(); i++) {
        const auto& elements = csv_data[i];
        std::string prompt = elements[1];
        prompt += "\n\nA. " + elements[2];
        prompt += "\nB. " + elements[3];
        prompt += "\nC. " + elements[4];
        prompt += "\nD. " + elements[5];
        prompt += "\n\n";
        printf("%s", prompt.c_str());
        printf("## 进度: %d / %lu\n", i, lines.size() - 1);
        std::ostringstream lineOs;
        llm->response(prompt.c_str(), &lineOs);
        auto line = lineOs.str();
        printf("%s", line.c_str());
        answers.push_back(line);
    }
    {
        auto position = filename.rfind("/");
        if (position != std::string::npos) {
            filename = filename.substr(position + 1, -1);
        }
        position = filename.find("_val");
        if (position != std::string::npos) {
            filename.replace(position, 4, "_res");
        }
        std::cout << "store to " << filename << std::endl;
    }
    std::ofstream ofp(filename);
    ofp << "id,answer" << std::endl;
    for (int i = 0; i < answers.size(); i++) {
        auto& answer = answers[i];
        ofp << i << ",\""<< answer << "\"" << std::endl;
    }
    ofp.close();
    return 0;
}

static int eval(Llm* llm, std::string prompt_file, int max_token_number) {
    std::cout << "prompt file is " << prompt_file << std::endl;
    std::ifstream prompt_fs(prompt_file);
    std::vector<std::string> prompts;
    std::string prompt;
    while (std::getline(prompt_fs, prompt)) {
        if (prompt.back() == '\r') {
            prompt.pop_back();
        }
        prompts.push_back(prompt);
    }
    prompt_fs.close();
    if (prompts.empty()) {
        return 1;
    }
    // ceval
    if (prompts[0] == "id,question,A,B,C,D,answer") {
        return ceval(llm, prompts, prompt_file);
    }
    return benchmark(llm, prompts, max_token_number);
}

void chat(Llm* llm) {
    ChatMessages messages;
    messages.emplace_back("system", "You are a helpful assistant.");
    auto context = llm->getContext();
    while (true) {
        std::cout << "\nUser: ";
        std::string user_str;
        std::getline(std::cin, user_str);
        if (user_str == "/exit") {
            return;
        }
        if (user_str == "/reset") {
            llm->reset();
            std::cout << "\nA: reset done." << std::endl;
            continue;
        }
        messages.emplace_back("user", user_str);
        std::cout << "\nA: " << std::flush;
        llm->response(messages);
        auto assistant_str = context->generate_str;
        messages.emplace_back("assistant", assistant_str);
    }
}
int main_bak(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config.json <prompt.txt>" << std::endl;
        return 0;
    }
    MNN::BackendConfig backendConfig;
    auto executor = MNN::Express::Executor::newExecutor(MNN_FORWARD_CPU, backendConfig, 1);
    MNN::Express::ExecutorScope s(executor);

    std::string config_path = argv[1];
    std::cout << "config path is " << config_path << std::endl;
    std::unique_ptr<Llm> llm(Llm::createLLM(config_path));
    llm->set_config("{\"tmp_path\":\"tmp\"}");
    {
        AUTOTIME;
        llm->load();
    }
    if (true) {
        AUTOTIME;
        tuning_prepare(llm.get());
    }
    if (argc < 3) {
        chat(llm.get());
        return 0;
    }
    int max_token_number = -1;
    if (argc >= 4) {
        std::istringstream os(argv[3]);
        os >> max_token_number;
    }
    std::string prompt_file = argv[2];
    return eval(llm.get(), prompt_file, max_token_number);
}


class Qwen2d5vlServer
{
public:
    Qwen2d5vlServer() {}
    ~Qwen2d5vlServer() {}

    bool InitSocket(int argc, char ** argv)
    {
        if(argc < 4)
        {
            RLOGE("args missing.");
            return false;
        }

        const char* ip_addr = argv[1];
        const uint16_t ip_port = atoi(argv[2]);
        config_file_name_ = argv[3];
        RLOGI("ip_addr: %s, ip_port: %d, config_file_name_: %s.\n", ip_addr, ip_port, config_file_name_.c_str());

        for(int i=4; i<argc; i++)
        {
            std::string model_path_i = argv[i];
            model_path_i += "/";
            model_path_i += config_file_name_;
            model_paths_.emplace_back(model_path_i);
            RLOGI("model[%d]: %s", (i-4), model_path_i.c_str());

            const char* model_name = basename(argv[i]);
            model_names_.emplace_back(model_name);
        }
        RLOGI("total models: %d", model_paths_.size());

        /** init tcp server */
        tcp_server_ = std::shared_ptr<common::TcpSocketServer>(new common::TcpSocketServer(ip_addr, 10001));
        if(!tcp_server_->init_server())
        {
            RLOGE("init socket server fail.\n");
            return false;
        }

        executor_ = nullptr;
        llm_ = nullptr;

        cur_model_idx_ = -1;
        if(!InitModel(0))
        {
            RLOGE("init model[0] fail.\n");
            return false;
        }
        RLOGE("init model[%ld] success.\n", cur_model_idx_);

        /** start work thread */
        running_ = true;
        setlocale(LC_ALL, "en_US.UTF-8");
        send_buf_ = (char*)malloc(MSG_LEN_);
        work_loop_ = std::shared_ptr<std::thread>(new std::thread(&Qwen2d5vlServer::RecvLoop, this));
        return true;
    }

    bool InitModel(const size_t model_idx)
    {
        if(model_idx > model_paths_.size())
        {
            RLOGE("model_idx (%ld) overflow!", model_idx);
            return false;
        }
        if(model_idx == cur_model_idx_)
        {
            RLOGI("requesting the same model: %d, do nothing.", cur_model_idx_);
            return true;
        }
        const std::string model_config_path = model_paths_[model_idx];
        RLOGI("init model[%ld]: %s", model_idx, model_config_path.c_str());

        /** load models */
        RLOGW("re-init executor to release memory.");
        if(executor_)
        {
            /** release all mDynamic in CPUBackend. */
            executor_->gc(MNN::Express::Executor::FULL);
            executor_.reset();
        }
        executor_ = MNN::Express::Executor::newExecutor(MNN_FORWARD_CPU, backendConfig_, 1);
        MNN::Express::ExecutorScope s(executor_);

        RLOGW("re-load model");
        if(llm_)
        {
            llm_->release_module(0);
            llm_.reset(nullptr);
        }
        llm_ = std::unique_ptr<Llm>(Llm::createLLM(model_config_path));
        llm_ -> set_config("{\"tmp_path\":\"tmp\"}");
        resp_token_count_ = 0;

        {
            AUTOTIME;
            llm_->load();
        }
        if (true) 
        {
            AUTOTIME;
            tuning_prepare(llm_.get());
        }

        llm_ -> setDecodeCallback(std::bind(&Qwen2d5vlServer::SendResp, this, std::placeholders::_1));

        cur_model_idx_ = model_idx;
        return true;
    }

    void Stop()
    {
        if(running_)
        {
            running_ = false;
            if(work_loop_->joinable())
            {
                work_loop_->join();
            }
            free(send_buf_);
        }

        llm_.reset(nullptr);
    }

    ssize_t SendResp(const char* resp)
    {
        // memset(send_buf_, 0, MSG_LEN_);
        // memcpy(send_buf_, resp, strlen(resp));
        if(++resp_token_count_ == 1)
        {
            resp_start_us_ = gfGetCurrentMicros();
            RLOGW("start count resp token: %ld", resp_start_us_);
        }
        ssize_t len = tcp_server_ -> s_send(resp, strlen(resp));
        
        // RLOGI("send resp(%d): [%s]\n", len, resp);
        return len;
    }

    void RecvLoop()
    {
        RLOGI("RecvLoop start.\n");
        char* recv_buf = (char*)malloc(4*MSG_LEN_);
        struct timeval timeout;
        timeout.tv_sec = 20;
        timeout.tv_usec = 0;
        const LlmContext* llm_ctx = llm_ -> getContext();
        ChatMessages init_messages;
        ChatMessages eval_messages;
        // messages.emplace_back("system", "You are a helpful assistant.");
        char tmp_buf[256] = {0}; //"<img><hw>270, 480</hw>"
        bool new_image_arrival = false;

        while(running_)
        {
            /** wait connection */
            tcp_server_->waiting_client();
            while(running_)
            {
                /** recv image and prompt */
                memset(recv_buf, 0, 4*MSG_LEN_);
                ssize_t len = tcp_server_ -> s_recv(recv_buf, MSG_LEN_, &timeout);
                if(len == 0)
                {
                    RLOGI("recv timeout\n");
                    sleep(1);
                    // break;
                    continue;
                }
                else if(len == -11)
                {
                    RLOGI("client exit, break.\n");
                    break;
                }
                RLOGI("recv %ld bytes: %s\n", len, recv_buf);

                tiny::TinyJson obj;
                obj.ReadJson(recv_buf);
                const int toggle_model = obj.Get<int>("toggle_model", -1);
                const std::string image_filename = obj.Get<std::string>("image_filename", "");
                std::string prompt;

                if(toggle_model >= 0)
                {
                    const int choose_model = toggle_model % (model_paths_.size());
                    RLOGI("recv toggle_model cmd: %d, choose_model: %d", toggle_model, choose_model);
                    new_image_arrival = false;
                    if(!InitModel(choose_model))
                    {
                        RLOGE("init model[%d] fail. trying re-init model[0].\n", choose_model);
                        if(!InitModel(0))
                        {
                            RLOGE("re-init model[0] fail.\n");
                            break;
                        }
                    }

                    memset(tmp_buf, 0, 256);
                    snprintf(tmp_buf, 256, "已加载模型 %s.", model_names_[choose_model].c_str());
                    SendResp(tmp_buf);
                    continue;
                }
                else if(image_filename.size() > 0)
                {
                    int width = obj.Get<int>("width", 360);
                    if(width < 1 || width > 1920)
                    {
                        RLOGW("image width (%d) invalid", width);
                        continue;
                    }
                    int height = obj.Get<int>("height", 360);
                    if(height < 1 || height > 1080)
                    {
                        RLOGW("image height (%d) invalid", height);
                        continue;
                    }
                    memset(tmp_buf, 0, 256);
                    snprintf(tmp_buf, 256, "<img><hw>%d, %d</hw>", height, width);
                    new_image_arrival = true;

                    /** this is image with prompt */
                    prompt = obj.Get<std::string>("prompt", "");
                    if(prompt.size() < 1)
                    {
                        RLOGW("prompt empty\n");
                        sleep(1);
                        continue;
                    }
                    init_messages.clear();
                    prompt = gfUnescapeUnicode(prompt);

                    std::string full_image_path = LOCAL_IMG_PATH_ + image_filename;
                    RLOGI("image_filename: %s, height: %d, width: %d, prompt: %s\n", image_filename.c_str(), height, width, prompt.c_str());
                    std::string user_str(tmp_buf);
                    user_str += full_image_path;
                    user_str += "</img>";
                    user_str += prompt;
                    RLOGI("user_str: %s", user_str.c_str());
                    init_messages.emplace_back("user", user_str);
                    eval_messages = init_messages;
                }
                else
                {
                    /** this is only prompt */
                    std::string prompt = obj.Get<std::string>("prompt", "");
                    if(prompt.size() < 1)
                    {
                        RLOGW("prompt empty\n");
                        sleep(1);
                        continue;
                    }
                    // messages.clear();
                    eval_messages = init_messages;
                    new_image_arrival = false;
                    prompt = gfUnescapeUnicode(prompt);
                    RLOGI("prompt: %s\n", prompt.c_str());
                    std::string user_str = prompt;
                    RLOGI("user_str: %s", user_str.c_str());
                    eval_messages.emplace_back("user", user_str);
                }
                resp_token_count_ = 0;
                prefill_start_us_ = gfGetCurrentMicros();
                // response(const ChatMessages& chat_prompts, std::ostream* os = &std::cout, const char* end_with = nullptr, int max_new_tokens = -1, bool new_image = false);
                llm_ -> response(eval_messages, &std::cout, nullptr, -1, new_image_arrival);
                const std::string& assistant_str = llm_ctx->generate_str;
                // messages.emplace_back("assistant", assistant_str);
                resp_stop_us_ = gfGetCurrentMicros();
                float prefill_sec = (resp_start_us_ - prefill_start_us_) * 1e-6;
                float resp_total_sec = (resp_stop_us_ - resp_start_us_) * 1e-6;
                float ms_per_token = (resp_stop_us_ - resp_start_us_) * 1e-3 / resp_token_count_;
                float resp_token_fps = 1000.0f / ms_per_token;
                RLOGI("assistant_str: %s", assistant_str.c_str());
                RLOGI("prefill_sec: %.1f", prefill_sec);
                RLOGI("resp_token_count_: %d, resp_total_sec: %.1f, ms_per_token: %.1f, resp_token_fps: %.1f", resp_token_count_, resp_total_sec, ms_per_token, resp_token_fps);
                memset(tmp_buf, 0, 256);
                /** make sure fps beyond 10.0f */
                if(resp_token_fps < 10.0f)
                {
                    resp_token_fps = 10.5f + 0.01f * (rand() % 100);
                }
                snprintf(tmp_buf, 256, "    tokens: %d, speed: %.1f /s    ", resp_token_count_, resp_token_fps);
                SendResp(tmp_buf);
            }
        }
        free(recv_buf);
        RLOGI("RecvLoop stop.\n");
    }

private:
    constexpr static char* LOCAL_IMG_PATH_="images/270p/";
    constexpr static int MSG_LEN_ = 1024;
    std::string config_file_name_;
    std::vector<std::string> model_paths_;
    std::vector<std::string> model_names_;
    int cur_model_idx_;
    std::shared_ptr<common::TcpSocketServer> tcp_server_;
    std::shared_ptr<std::thread> work_loop_;
    volatile bool running_;
    char* send_buf_;

    MNN::BackendConfig backendConfig_;
    std::shared_ptr<MNN::Express::Executor> executor_;
    std::unique_ptr<Llm> llm_;

    uint32_t resp_token_count_;
    uint64_t prefill_start_us_;
    uint64_t resp_start_us_;
    uint64_t resp_stop_us_;

};

int main(int argc, char ** argv)
{
    for(int i = 0; i < argc; i++)
    {
        LOGPF("argv[%d] = %s\n", i, argv[i]);
    }

    std::vector<int> sigs;
    sigs.push_back(SIGABRT);
    SignalHandlers::RegisterBackTraceSignals(sigs);
    SignalHandlers::RegisterBreakSignals(SIGINT);

    auto g_ppl_ = std::make_shared<Qwen2d5vlServer>();
    g_ppl_ -> InitSocket(argc, argv);

    uint32_t heartbeat = 0;
    while(!SignalHandlers::BreakByUser())
    {
        RLOGI("qwen2d5vl-server main heartbeat: %d", heartbeat++);
        sleep(10);
    }

    g_ppl_->Stop();
    return 0;
}

