#include <Magick++.h> 
#include <json.hpp>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <vector>
#include <algorithm>
#include <math.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <iterator>
#include <set>
#include <string>

typedef std::vector<boost::filesystem::directory_entry> path_listing;
typedef std::vector<boost::filesystem::path> extension_list;

const size_t max_directory_depth = 10;

struct image_meta{
  std::string checksum;
  std::string exif_date_original;
};

bool operator<(const image_meta &a,const image_meta &b)
{
  return a.checksum < b.checksum;
}

void to_json(nlohmann::json& j, const image_meta& im) {
   j = nlohmann::json{{"id", im.checksum}, {"date", im.exif_date_original}};
}

void from_json(const nlohmann::json& j, image_meta& im) {
        
        im.checksum = j.at("id").get<std::string>();
        im.exif_date_original= j.at("date").get<std::string>();
}

typedef std::set<image_meta> image_set;

bool calculate_checksum(boost::filesystem::path filep, std::string &checksum)
{
  std::ifstream file(filep.string(),std::ios::in | std::ios::binary);
  std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
 
  boost::crc_32_type result;
  result.process_bytes(&raw[0],raw.size());

  std::stringstream s;
  s << std::hex << result.checksum();
  s >> checksum;

  return true;
}


bool list_directory(const boost::filesystem::path root_directory,path_listing &files, extension_list extensions, size_t recursions = 0)
{
  recursions++;
  bool is_directory = true;

  if(boost::filesystem::exists(root_directory) && boost::filesystem::is_directory(root_directory))   {
    for(auto &entry: boost::filesystem::directory_iterator(root_directory))
      {
        if(boost::filesystem::is_directory(entry))
        {
          if(recursions <= max_directory_depth)
          {
            list_directory(entry.path(),files,extensions, recursions);
          }
        }

        if(boost::filesystem::is_regular_file(entry) && std::find(extensions.begin(),extensions.end(),entry.path().extension()) != extensions.end())
        {
          files.push_back(entry);
        } 
      }

  }else
  {
    is_directory = false;
  }

  return is_directory;

}

bool resize_single_picture(boost::filesystem::directory_entry entry, boost::filesystem::path output_directory, int new_width, std::string checksum, image_set &pictures_added)
{
   if(!boost::filesystem::is_directory(output_directory))
  {
    return false;
  }

   std::string filepath = entry.path().string();

   Magick::Image image;

   image.read(filepath); 
   auto size = image.size();
  
   double w = size.width();
   double h = size.height();
   
   double ratio = h/w;
   int new_height = floor(new_width * ratio);

   std::string file_with_extension = checksum + entry.path().extension().string();
   auto output_filepath = output_directory / std::to_string(new_width) / file_with_extension;
   std::string output_name(output_filepath.string());

  

   std::cout << "Resizing..." << output_name << std::endl;

   //image.filterType(Magick::FilterType::LanczosFilter); // already default + not supported in earlier lib versions
   image.resize(Magick::Geometry(new_width, new_height));
  
   image.write(output_name);

   image_meta imgi;
   imgi.checksum = checksum;
   imgi.exif_date_original = image.attribute("EXIF:DateTimeOriginal");
   
   pictures_added.insert(imgi);
   return true; 
}

bool create_direectories()
{
  return true;
}

bool resize_pictures_in_directory(path_listing files, boost::filesystem::path output_directory, int new_width, image_set &pictures_added)
{
  std::string checksum = "";

  for(auto &entry : files)
  {
    calculate_checksum(entry.path(), checksum);
    resize_single_picture(entry, output_directory, new_width, checksum, pictures_added);
  }

  return true;
}

bool parallel_resize(path_listing files, boost::filesystem::path output_directory, int new_width, image_set &pictures_added)
{

  size_t cores = std::thread::hardware_concurrency()/2;

  int batch_size = files.size()/cores;
  int start = 0;
  int stop = batch_size;

  std::vector<std::thread> thread_pool;

  for(int p=0;p<cores;p++)
  {
    path_listing partitioned_files(files.begin()+start, files.begin()+stop);
    start += batch_size;
    stop += batch_size;

    thread_pool.push_back(std::thread(resize_pictures_in_directory,partitioned_files,output_directory,new_width, std::ref(pictures_added)));  
  }

  for(auto &t : thread_pool)
  {
    t.join(); 
  }
  return true;

}

bool load_json(boost::filesystem::path path,nlohmann::json &json)
{
  path = path / "digest.json";
  std::ifstream j(path.string());
  if(j)
  {
    j >> json;
    return true;
  } 

  return false;
}

bool save_json(boost::filesystem::path path, nlohmann::json json)
{
  path = path / "digest.json";
  std::ofstream j(path.string());
  if(j)
  {
  std::cout << "SAVING" << path.string() << std::endl;
    j << json.dump(4); 
    return true; 
  }

    return false;
}

int main(int argc, char** argv)
{

nlohmann::json json;

bool existing_json = load_json(boost::filesystem::path(argv[2]),json);
path_listing files;

extension_list allowed_extensions({".jpg",".jpeg",".JPG",".png",".PNG"});

image_set pictures_added;
std::set<int> resolutions;

try
{
image_set pa = json[argv[3]]["pictures"];
std::set<int> res = json[argv[3]]["widths"];

pictures_added = pa;
resolutions = res;
}catch(std::exception &e)
{
}


list_directory(boost::filesystem::path(argv[1]),files, allowed_extensions);
if(argc > 4)
{
  for(int p=4; p< argc;p++)
  {
    boost::filesystem::path sub = boost::filesystem::path(argv[2]) / std::to_string(std::stoi(argv[p]));

    boost::filesystem::create_directory(sub);
    resolutions.insert(std::stoi(argv[p]));
    resize_pictures_in_directory(files,boost::filesystem::path(argv[2]),std::stoi(argv[p]), pictures_added);
   // parallel_resize(files,boost::filesystem::path(argv[2]),std::stoi(argv[p]), pictures_added);;
  }
}



json[argv[3]]["pictures"] = nlohmann::json(pictures_added);
json[argv[3]]["widths"] = resolutions;

save_json(boost::filesystem::path(argv[2]), json);

return 0;
}
