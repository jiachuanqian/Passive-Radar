#include "typedefs.h"


void WGS2ENU(double GPS_tower[3], double GPS_base[3], double* xyz)
{
    double RLat_tower = GPS_tower[0] * pi / 180;
    double RLong_tower = GPS_tower[1] * pi / 180;
    double N_tower = a / sqrt(1 - e * e * sin(RLat_tower) * sin(RLat_tower));
    double ECEFPos_tower[3] = { 0 };
    ECEFPos_tower[0] = (N_tower + GPS_tower[2]) * cos(RLat_tower) * cos(RLong_tower);
    ECEFPos_tower[1] = (N_tower + GPS_tower[2]) * cos(RLat_tower) * sin(RLong_tower);
    ECEFPos_tower[2] = (N_tower * (1 - e * e) + GPS_tower[2]) * sin(RLat_tower);

    double RLat_base = GPS_base[0] * pi / 180;
    double RLong_base = GPS_base[1] * pi / 180;
    double N_base = a / sqrt(1 - e * e * sin(RLat_base) * sin(RLat_base));

    double ECEFPos_base[3]{};
    ECEFPos_base[0] = (N_base + GPS_base[2]) * cos(RLat_base) * cos(RLong_base);
    ECEFPos_base[1] = (N_base + GPS_base[2]) * cos(RLat_base) * sin(RLong_base);
    ECEFPos_base[2] = (N_base * (1 - e * e) + GPS_base[2]) * sin(RLat_base);

    double ECEFPos[3] = { 0 };
    for (int i = 0; i < 3; i++)
    {
        ECEFPos[i] = ECEFPos_tower[i] - ECEFPos_base[i];
    }

    double Rne[9] = { 0 };
    Rne[0] = -sin(RLong_base);
    Rne[1] = cos(RLong_base);
    Rne[2] = 0;
    Rne[3] = -sin(RLat_base) * cos(RLong_base);
    Rne[4] = -sin(RLat_base) * sin(RLong_base);
    Rne[5] = cos(RLat_base);
    Rne[6] = cos(RLat_base) * cos(RLong_base);
    Rne[7] = cos(RLat_base) * sin(RLong_base);
    Rne[8] = sin(RLat_base);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            xyz[i] += Rne[i * 3 + j] * ECEFPos[j];
    }
}
void ReadPhaseFile(float* array, int size, const std::string& filename)
{
    std::ifstream file(filename); 
    if (file.is_open()) { 
        for (int i = 0; i < size; i++) {
            if (!(file >> array[i])) { 
                std::cout << "error ch number to read phase" << std::endl;
                break;
            }
        }
        file.close(); 
    }
    else {
        std::cout << "unable to read file" << std::endl;
    }
}


void GetFiles(std::string path, std::vector<std::string>& files)
{
    if (!std::filesystem::exists(path)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            if (entry.path().filename() != "." && entry.path().filename() != "..") {
                GetFiles(entry.path().string(), files);
            }
        }
        else {
            files.push_back(entry.path().string());
        }
    }
    //std::sort(files.begin(), files.end(), numericalCompare);
}


size_t GetLength(std::string filename, int n)
{
    std::ifstream ifs(filename, std::ios::binary);
    size_t fileslength{};
    if (ifs) {
        ifs.seekg(0, std::ios::end);
        fileslength = ifs.tellg() / sizeof(int) - n / sizeof(int);
        ifs.seekg(0, std::ios::beg);
        ifs.close();
    }
    else {
        std::cout << "unable to read file" << std::endl;
    }
    return fileslength;
}

void ReadFile(std::string filename, void* data, int size, size_t start)
{
    std::ifstream ifs(filename, std::ios::binary);
    if (ifs) {
        ifs.seekg(start, std::ios::beg);
        ifs.read(reinterpret_cast<char*>(data), size);
        ifs.close();
    }
    else {
        std::cout << "unable to read file" << std::endl;
    }
}




void ENU2WGS(double GPS_base[3], double* xyz)
{
    double RLat_base = GPS_base[0] * pi / 180;
    double RLong_base = GPS_base[1] * pi / 180;
    double RneT[9] = { 0 };
    RneT[0] = -sin(RLong_base);
    RneT[1] = -sin(RLat_base) * cos(RLong_base);
    RneT[2] = cos(RLat_base) * cos(RLong_base);
    RneT[3] = cos(RLong_base);
    RneT[4] = -sin(RLat_base) * sin(RLong_base);
    RneT[5] = cos(RLat_base) * sin(RLong_base);
    RneT[6] = 0;
    RneT[7] = cos(RLat_base);
    RneT[8] = sin(RLat_base);
    double Pe[3] = { 0 };
    double ECEFPos_base[3]{};
    double N_base = a / sqrt(1 - e * e * sin(RLat_base) * sin(RLat_base));
    ECEFPos_base[0] = (N_base + GPS_base[2]) * cos(RLat_base) * cos(RLong_base);
    ECEFPos_base[1] = (N_base + GPS_base[2]) * cos(RLat_base) * sin(RLong_base);
    ECEFPos_base[2] = (N_base * (1 - e * e) + GPS_base[2]) * sin(RLat_base);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            Pe[i] += RneT[i * 3 + j] * xyz[j];
        Pe[i] += ECEFPos_base[i];
    }
    double b = std::sqrt(a * a * (1.0 - e * e));
    double ep = std::sqrt((a * a - b * b) / (b * b));
    double p = std::hypot(Pe[1], Pe[0]);
    double th = std::atan2(a * Pe[2], b * p);

    xyz[0] = atan2(Pe[1], Pe[0]) * 180 / pi;
    xyz[1] = atan2((Pe[2] + ep * ep * b * sin(th) * sin(th) * sin(th)), (p - e * e * a * cos(th) * cos(th) * cos(th))) * 180 / pi;

    double N = a / std::sqrt(1.0 - e * e * sin(xyz[1] * pi / 180) * sin(xyz[1] * pi / 180));

    if (abs(xyz[1] * pi / 180) < pi / 4.0)
    {
        xyz[2] = p / cos(xyz[1] * pi / 180) - N;
    }
    else
    {
        xyz[2] = Pe[2] / sin(xyz[1] * pi / 180) - N * (1.0 - e * e);
    }

}

//void cfar(std::complex<float>* out, size_t idx, size_t TM, size_t TN)
//{
//    //std::fill(out + TN * (TM / 2 - 5), out + TN * (TM / 2 + 6), *std::min_element(out, out + TM * TN));
//
//    double sum{};
//    for (size_t j = 0; j < TM; ++j)
//    {
//        for (size_t j = 0; j < TN; ++j)
//        {
//
//        }
//    }
//
//}


float cfar(std::complex<float>* out, size_t n_beams, size_t TM, size_t TN, size_t& beam_idx, size_t& idx)
{
    const size_t per_beam = TM * TN;
    const size_t noise_rows = (TM - 1) / 4;
    const size_t noise_count = noise_rows * TN;
    const int center_start_row = static_cast<int>(TM) / 2 - 21;
    const int center_end_row = static_cast<int>(TM) / 2 + 21;
    const int center_start_row_ = static_cast<int>(TM) / 2 - 50;
    const int center_end_row_ = static_cast<int>(TM) / 2 + 50;

    float max_snr = -std::numeric_limits<float>::infinity();
    beam_idx = 0;
    idx = 0;

    for (size_t beam = 0; beam < n_beams; ++beam) {
        std::complex<float>* beam_data = out + beam * per_beam;

        float noise_sum = 0.0f;
        for (size_t r = 0; r < noise_rows; ++r) {
            for (size_t c = 0; c < TN; ++c) {
                size_t offset = r * TN + c;
                noise_sum += std::norm(beam_data[offset]);
            }
        }

        const float noise_avg = noise_sum / static_cast<float>(noise_count);

        const float row_thresh = 10 * log10(noise_avg * std::pow(10.0f, -0.1f)); // noise - 1 dB

        for (size_t r = 0; r < TN; ++r) {
            float row_sum = 0.0f;
            for (size_t c = 0; c < TM; ++c) {
                row_sum += 10 * log10(std::norm(beam_data[c * TN + r]));
            }
            float row_avg = row_sum / static_cast<float>(TM);

            if (row_avg > row_thresh) {
                for (size_t c = 0; c < TM; ++c) {
                    beam_data[c * TN + r] = std::complex<float>(std::sqrt(noise_avg), 0.0f);
                }
            }
        }



        for (size_t r = 0; r < TM; ++r) {
            if ((static_cast<int>(r) > center_start_row &&
                static_cast<int>(r) < center_end_row)||
                static_cast<int>(r) < center_start_row_||
                static_cast<int>(r) > center_end_row_)
            {
				//std::cout << "skip row: " << r << std::endl;
                continue;
            }

            for (size_t c = 0; c < TN; ++c) {
                if (c < 30) {
                    continue;
                }
                size_t offset = r * TN + c;
                float power = std::norm(beam_data[offset]);

                float snr = power / noise_avg;

                if (snr > max_snr) {
                    max_snr = snr;
                    beam_idx = beam;
                    idx = offset;
                }
            }
        }
        
        //std::string filename =  ".oz";

        //std::ofstream outfile(filename, std::ios::binary);
        //if (outfile.is_open()) {
        //    outfile.write(reinterpret_cast<const char*>(beam_data), TM*TN * sizeof(std::complex<float>));
        //    outfile.close();
        //}
    }
    return 10 * log10(max_snr);
}
    //std::cout << "SNR: " << std::fixed << std::setprecision(4) << 10 * log10(max_snr) << " ---------";

std::vector<DetectionResult> cfar_(std::complex<float>* out, size_t n_beams, size_t TM, size_t TN, float threshold_db)
{
    const size_t per_beam = TM * TN;
    const size_t noise_rows = (TM - 1) / 4;
    const size_t noise_count = noise_rows * TN;
    const int center_start_row = static_cast<int>(TM) / 2 - 10;
    const int center_end_row = static_cast<int>(TM) / 2 + 11;

    std::map<size_t, DetectionResult> results_map; // key: idx, value: DetectionResult

    for (size_t beam = 0; beam < n_beams; ++beam) {
        const std::complex<float>* beam_data = out + beam * per_beam;

        float noise_sum = 0.0f;
        for (size_t r = 0; r < noise_rows; ++r) {
            for (size_t c = 0; c < TN; ++c) {
                size_t offset = r * TN + c;
                noise_sum += std::norm(beam_data[offset]);
            }
        }

        float noise_avg = noise_sum / static_cast<float>(noise_count);

        if (noise_avg <= 0.0f) continue;

        for (size_t r = 0; r < TM; ++r) {
            if (static_cast<int>(r) >= center_start_row &&
                static_cast<int>(r) < center_end_row) {
                continue;
            }

            for (size_t c = 0; c < TN; ++c) {
                if (c < 30) {
                    continue;
                }

                size_t offset = r * TN + c;
                float power = std::norm(beam_data[offset]);

                float snr = 10 * log10(power / noise_avg);

                if (snr > threshold_db) {

                    auto it = results_map.find(offset);
                    if (it == results_map.end() || snr > it->second.snr) {
                        results_map[offset] = DetectionResult(beam, offset, snr);
                    }
                }
            }
        }
    }
    std::vector<DetectionResult> results;
    results.reserve(results_map.size());
    for (const auto& pair : results_map) {
        results.push_back(pair.second);
    }
    //std::cout << "results[1].snr = " << results[1].snr << std::endl;
    return results;
}

void save_results_to_csv(const std::string& filename,
    int call_number,
    const std::vector<DetectionResult>& results,
    bool append_header) 
{
    std::ofstream file;

    if (append_header) {
        file.open(filename, std::ios::out); 
        file << "Call Number,Beam ID,Index,SNR\n";
    }
    else {
        file.open(filename, std::ios::app); 
    }

    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }

    for (const auto& result : results) {
        file << call_number << ","
            << result.beam_idx << ","
            << result.idx << ","
            << result.snr << "\n";
    }

    file.close();
}