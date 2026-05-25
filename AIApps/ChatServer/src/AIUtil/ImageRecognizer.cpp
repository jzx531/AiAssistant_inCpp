#include "../include/AIUtil/ImageRecognizer.h"

ImageRecognizer::ImageRecognizer(const std::string& model_path,
    const std::string& label_path)
    : env(ORT_LOGGING_LEVEL_WARNING, "ImageRecognizer")
{
    // 配置 ONNX Runtime 会话参数。
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // 加载模型并初始化默认内存分配器。
    session = std::make_unique<Ort::Session>(env, model_path.c_str(), session_options);
    allocator = std::make_unique<Ort::AllocatorWithDefaultOptions>();


    /*
     * 获取模型输入输出节点名称，后续推理时需要按名称绑定张量。
     */
    input_name = session->GetInputNameAllocated(0, *allocator).get();
    output_name = session->GetOutputNameAllocated(0, *allocator).get();


    // 读取模型输入张量形状，通常格式为 [N, C, H, W]。
    input_shape = session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
    input_height = static_cast<int>(input_shape[2]);
    input_width = static_cast<int>(input_shape[3]);


    // 加载类别标签文件。
    LoadLabels(label_path);
}

void ImageRecognizer::LoadLabels(const std::string& label_path) {
    std::ifstream infile(label_path);
    if (!infile.is_open()) {
        throw std::runtime_error("Failed to open label file: " + label_path);
    }

    std::string line;
    while (std::getline(infile, line)) {
        // 忽略空行，避免产生无效标签。
        if (!line.empty()) {
            labels.push_back(line);
        }
    }
    infile.close();

    if (labels.empty()) {
        throw std::runtime_error("No labels loaded from file: " + label_path);
    }
}

std::string ImageRecognizer::PredictFromFile(const std::string& image_path) {
    // 从磁盘读取图片，再复用 Mat 推理流程。
    cv::Mat img = cv::imread(image_path);
    if (img.empty()) {
        throw std::runtime_error("Failed to load image: " + image_path);
    }
    return PredictFromMat(img);
}

std::string ImageRecognizer::PredictFromBuffer(const std::vector<unsigned char>& image_data) {
    // 将内存中的图片字节流解码为 OpenCV 图像对象。
    cv::Mat img = cv::imdecode(image_data, cv::IMREAD_COLOR);
    if (img.empty()) {
        throw std::runtime_error("Failed to decode image from buffer");
    }
    return PredictFromMat(img);
}

std::string ImageRecognizer::PredictFromMat(const cv::Mat& img_raw) {
    if (img_raw.empty()) {
        throw std::runtime_error("Input image is empty");
    }

    /*
     * 先将图像缩放到模型要求的输入尺寸，
     * 再把像素值归一化到 [0, 1] 区间。
     */
    cv::Mat img;
    cv::resize(img_raw, img, cv::Size(input_width, input_height));
    img.convertTo(img, CV_32F, 1.0 / 255.0);

    // 将 OpenCV 图像转换为模型常用的 NCHW 张量布局。
    cv::dnn::blobFromImage(img, img);

    // 构造输入张量维度信息。
    std::vector<int64_t> dims = { 1, 3, input_height, input_width };
    size_t input_tensor_size = 1 * 3 * input_height * input_width;

    // 创建位于 CPU 内存中的输入张量。
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, img.ptr<float>(), input_tensor_size, dims.data(), dims.size());

    // 执行一次前向推理。
    const char* input_names[] = { input_name.c_str() };
    const char* output_names[] = { output_name.c_str() };

    auto output_tensors = session->Run(
        Ort::RunOptions{ nullptr },
        input_names, &input_tensor, 1,
        output_names, 1
    );

    float* output_data = output_tensors.front().GetTensorMutableData<float>();

    // 取概率最大的类别下标作为预测结果。
    int num_classes = labels.empty() ? 1000 : (int)labels.size();
    int pred_class = std::max_element(output_data, output_data + num_classes) - output_data;

    // 若下标有效则返回标签名称，否则返回兜底值。
    if (pred_class >= 0 && pred_class < (int)labels.size()) {
        return labels[pred_class];
    }
    else {
        return "Unknown";
    }
}
