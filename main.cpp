#include <Magick++.h> 
#include <json.hpp>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <vector>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <thread>
#include <iterator>

typedef std::vector<boost::filesystem::directory_entry> path_listing;

typedef std::vector<boost::filesystem::path> extension_list;

bool calculate_checksum(boost::filesystem::path filep, uint32_t &checksum)
{


  std::ifstream file(filep.string(),std::ios::in | std::ios::binary);
  std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
 
  boost::crc_32_type result;
  result.process_bytes(&raw[0],raw.size());
  checksum = result.checksum();

  return true;
}


bool list_directory(const boost::filesystem::path root_directory,path_listing &files, path_listing &subdirectories, extension_list extensions)
{

  bool is_directory = true;

  if(boost::filesystem::exists(root_directory) && boost::filesystem::is_directory(root_directory))   {
    for(auto &entry: boost::filesystem::directory_iterator(root_directory))
      {
        if(boost::filesystem::is_directory(entry))
        {
          subdirectories.push_back(entry);         
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

bool resize_single_picture(boost::filesystem::directory_entry entry, boost::filesystem::path output_directory, int new_width, uint32_t checksum)
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

   std::stringstream s;
   s << std::hex << checksum << entry.path().extension().string();
   
   std::string hex_name;
   s >> hex_name; 
 
   auto output_filepath = output_directory / hex_name;
   std::string output_name(output_filepath.string());

   std::cout << "Resizing..." << output_name << std::endl;

   image.filterType(Magick::FilterType::LanczosFilter);
   image.resize(Magick::Geometry(new_width, new_height));
  
   image.write(output_name);
   return true; 

}

bool resize_pictures_in_directory(path_listing files, boost::filesystem::path output_directory, int new_width)
{
  uint32_t checksum = 0;

  for(auto &entry : files)
  {
    calculate_checksum(entry.path(), checksum);
    resize_single_picture(entry, output_directory, new_width, checksum);
  }

  return true;
}

bool parallel_resize(path_listing files, boost::filesystem::path output_directory, int new_width)
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

    thread_pool.push_back(std::thread(resize_pictures_in_directory,partitioned_files,output_directory,new_width));  
  }

  for(auto &t : thread_pool)
  {
    t.join(); 
  }
  return true;

}

int main(int argc, char** argv)
{

using json = nlohmann::json;

path_listing files, subdirectories;

extension_list allowed_extensions({".jpg",".jpeg",".JPG",".png",".PNG"});

list_directory(boost::filesystem::path(argv[1]),files, subdirectories, allowed_extensions);
resize_pictures_in_directory(files,boost::filesystem::path(argv[2]),std::stoi(argv[3]));
//parallel_resize(files,boost::filesystem::path(argv[2]),std::stoi(argv[3]));

return 0;
}
