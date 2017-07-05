#include <CImg.h>
#include <json.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <algorithm>
#include <math.h>
#include <iostream>


typedef std::vector<boost::filesystem::directory_entry> path_listing;

typedef std::vector<boost::filesystem::path> extension_list;

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

bool resize_pictures_in_directory(path_listing files, boost::filesystem::path output_directory, int new_width)
{
  if(!boost::filesystem::is_directory(output_directory))
  {
    return false;
  }  

  int i = 0;

  for(auto &entry : files)
  {
   i++;
   std::string filepath = entry.path().string();
   
   cimg_library::CImg<uint8_t> img(filepath.c_str());
   double w = img.width();
   double h = img.height();
 
   double ratio = h/w;
   int new_height = floor(new_width * ratio);
  
   auto output_filepath = output_directory / entry.path().filename();
   std::string output_name(output_filepath.string());
   
   std::cout << "Resizing..." << output_name << std::endl;

   img.resize(new_width,new_height,1,3,6);
   img.save(output_name.c_str(),i,3);   
    
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

return 0;
}
