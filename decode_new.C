#include <TTree.h>
#include <TString.h>
#include <Riostream.h>
#include <TFile.h>
#include <regex>
#include <fstream>
#include <iostream>

std::pair<int, int> get_orbit_bxid(const std::string &line)
{
  int orbit, bxid;
  const std::regex event_regex("Event: [0-9]+ \\(or orbit:([0-9]+) and bxid:([0-9]+) in old MDF\\)");

  // Extraction of a sub-match
  std::smatch base_match;

  if (std::regex_match(line, base_match, event_regex))
  {
    // The first sub_match is the whole string; the next
    // sub_match is the first parenthesized expression.
    if (base_match.size() == 3)
    {
      std::ssub_match orbit_match = base_match[1];
      std::ssub_match bxid_match = base_match[2];

      orbit = std::stoi(orbit_match.str());
      bxid = std::stoi(bxid_match.str());
    }
    else
    {
      std::cout << "Trying to math the wrong line!!!" << std::endl;
      return {-1, -1};
    }
  }

  return {orbit, bxid};
};

bool is_error_header(std::ifstream &mainfile, std::string &line)
{
  for (int i = 0; i < 3; i++)
  {
    getline(mainfile, line);
    if (line.find("DaqError") != std::string::npos)
    {
      getline(mainfile, line);
      return true;
    }
  }
  return false;
};


std::string read_bank(std::ifstream &mainfile, std::string &line)
{
  TString tmp_line = "";
  for (int j = 0; j < 5; j++)
  {
    getline(mainfile, line);
    tmp_line += (TString)line;
  }
  tmp_line.ReplaceAll(" ", "");
  tmp_line.ReplaceAll("|", "");
  tmp_line.ReplaceAll("0x0000", "");
  tmp_line.ReplaceAll("0x0020", "");
  tmp_line.ReplaceAll("0x0040", "");
  tmp_line.ReplaceAll("0x0060", "");
  tmp_line.ReplaceAll("0x0080", "");
  
  return (std::string)tmp_line;
}


void decode_new(TString inputfile_name = "/daqarea1/plume/Run_0000222414_20211029-235108-893_PLEB01.txt")
{

  //28/10/21
  //std::ifstream mainfile("/daqarea1/plume/0000222089/Run_0000222089_20211029-040618-060_PLEB01.txt");
  //29/10/21
  //TString inputfile_name = "/daqarea1/plume/Run_0000222414_20211029-235108-893_PLEB01.txt";
  //inputfile_name = "/daqarea1/plume/Run_0000222414_20211029-235110-510_SODIN01.txt";
  TString outputfile_name = inputfile_name;
  outputfile_name.ReplaceAll(".txt", ".root");
  std::ifstream mainfile(inputfile_name);
  TFile *outfile = new TFile(outputfile_name, "recreate");

  TTree *tree = new TTree("plume", "plume");

  Int_t bxid, orbit, evtid;
  Int_t ch_str0_feb[96];
  Int_t ch_str1_feb[96];

  // Int_t etmax[10], add[5], ettot[11], mult[6];

  tree->Branch("evtid", &evtid, "evtid/I");
  tree->Branch("bxid", &bxid, "bxid/I");
  tree->Branch("orbit", &orbit, "orbit/I");
  tree->Branch("ch_str0_feb", ch_str0_feb, "ch_str0_feb[96]/I");
  tree->Branch("ch_str1_feb", ch_str1_feb, "ch_str1_feb[96]/I");

  std::vector<int> mydata;

  int evt = 0;
  std::string line, odinline;
  std::string head, data;
  std::string str_bxid;
  std::string str_evtid;
  std::string str_orbit;
  std::string str_tae;
  std::string str_tae_center;
  int ntruncated = 0;
  int nsp = 0;
  int m_nfeb = -1;
  Int_t nfeb = -1;

  std::string::size_type sz;

  bool good_event = false;
  int linecount = 0;
  while (getline(mainfile, line))
  {
    linecount++;
    if (line.find("DaqError") != std::string::npos)
    {
      std::cout << "daqerror file found" << std::endl;
      good_event = false;
      ntruncated++;
      continue;
    }

    // std::istringstream ss(line);
    // std::string lab;
    // size_t pos;
    // ss >> lab;
    //
    //cout << lab << endl;

    // std::cout << "here1" << std::endl;
    //      cout << line << endl;
    if (line.find("event_id") == 7)
    {
      //cout << "here1bis" << endl;
      good_event = false;
      // cout << line << endl;
      str_evtid = line.substr(16, 6);
      evtid = stoi(str_evtid);
      //cout << "here2" << endl;
      // std::cout << "EVT ID: " << evtid << std::endl;
    }
    //  Event: 22958867939855377 (or orbit:5345528 and bxid:3089 in old MDF)

    if (line.find("Event") == 0)
    {
      auto orbit_bxid = get_orbit_bxid(line);
      orbit = orbit_bxid.first; bxid = orbit_bxid.second;
      std::cout << line << std::endl;
      // std::cout << orbit << " " << bxid << std::endl;
    }


    if (line.find("Bank: 0x5001") == 1)
    {

      if (is_error_header(mainfile, line)){
        // std::cout << "ERROR HEADER" << line << std::endl;
        continue;
      }

      std::string bank = read_bank(mainfile, line);
      // std::cout << bank << std::endl;
      // std::cout << "hereA" << std::endl;
      // ignore the first 24 char as they are LLT data
      for (int k = 0; k < 96; k++)
      {
        ch_str0_feb[k] = stoi(bank.substr(24 + 3 * k, 3), 0, 16);
        if (ch_str0_feb[k] != 0 && ch_str0_feb[k] != 4095 && ch_str0_feb[k] > 500)
        {
          std::cout << ch_str0_feb[k] << std::endl;
          std::cout << "STREAM 0: " << k << " bxid " << bxid << std::endl;
          good_event = true;
        }
        //if(ch_str0_feb[k]!=0 && ch_str0_feb[k]!=4095) cout << "stream 0 " << k << endl;
        //      cout << line.substr(24+3*k,3) << " " << ch_str0_feb[k] << endl;
      }
    }

    if (line.find("Bank: 0x5002") == 1)
    {
      if (is_error_header(mainfile, line)){
        // std::cout << "ERROR HEADER" << line << std::endl;
        continue;
      }

      std::string bank = read_bank(mainfile, line);

      // std::cout << bank << std::endl;
      //cout << line << endl;
      // ignore the first 24 char as they are LLT data
      for (int k = 0; k < 96; k++)
      {
        ch_str1_feb[k] = stoi(bank.substr(24 + 3 * k, 3), 0, 16);
        //cout << ch_str1_feb[k] << endl;
        if (ch_str1_feb[k] != 0 && ch_str1_feb[k] != 4095 && ch_str1_feb[k] > 500)
        {
          std::cout << ch_str1_feb[k] << std::endl;
          std::cout << "STREAM 1: " << k << " bxid " << bxid << std::endl;
          good_event = true;
        }
        //        cout << line.substr(24+3*k,3) << " " << ch_str1_feb[k] << endl;
      }


    }

      if (good_event)
      {
        std::cout << "good event " << evtid << std::endl;
        std::cout << "Line number " << linecount << std::endl;
        tree->Fill();
        // if(tree->GetEntries()==2000) break;
        good_event = false;
      }
  }

  //  outfile->cd();
  tree->Write("", TObject::kOverwrite);
  outfile->Close();
  mainfile.close();
}