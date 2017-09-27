/*

EGYPT Toolkit for Statistical Machine Translation
Written by Yaser Al-Onaizan, Jan Curin, Michael Jahr, Kevin Knight, John Lafferty, Dan Melamed, David Purdy, Franz Och, Noah Smith, and David Yarowsky.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
USA.

*/


// new main.cpp

#include <sstream>
#include "getSentence.h"
#include "TTables.h"
#include "model1.h"
#include "model2.h"
#include "model3.h"
#include "hmm.h"
#include "file_spec.h"
#include "defs.h"
#include "vocab.h"
#include "Perplexity.h"
#include "Dictionary.h"
#include "utility.h" 
#include "Parameter.h"
#include "myassert.h"
#include "D4Tables.h"
#include "D5Tables.h"
#include "transpair_model4.h"
#include "transpair_model5.h"

#define ITER_M2 0
#define ITER_MH 5

/**
这些宏定义语句的作用是：1.创建一个类型为TYP的全局变量VARNAME，赋值为INIT  2.利用NAME,ParamterChangedFlag e.t.创建一个Parameter对象，然后返回指向
它的指针，类型转换后(const引用)，插入ParSet(局部静态)中[对于第二点注意如果该全局变量对应有多个名字，则对每个名字都会对应new一个Parameter，返回指针插入ParSet]。
#define GLOBAL_PARAMETER(TYP,VARNAME,NAME,DESCRIPTION,LEVEL,INIT) TYP VARNAME=addGlobalParameter< TYP >(NAME,DESCRIPTION,LEVEL,&VARNAME,INIT);//这里的&VARNAME是多出来的
#define GLOBAL_PARAMETER2(TYP,VARNAME,NAME,NAME2,DESCRIPTION,LEVEL,INIT) TYP VARNAME=addGlobalParameter< TYP >(NAME,NAME2,DESCRIPTION,LEVEL,&VARNAME,INIT);
#define GLOBAL_PARAMETER3(TYP,VARNAME,NAME,NAME2,NAME3,DESCRIPTION,LEVEL,INIT) TYP VARNAME=addGlobalParameter< TYP >(NAME,NAME2,NAME3,DESCRIPTION,LEVEL,&VARNAME,INIT);
#define GLOBAL_PARAMETER4(TYP,VARNAME,NAME,NAME2,NAME3,NAME4,DESCRIPTION,LEVEL,INIT) TYP VARNAME=addGlobalParameter< TYP >(NAME,NAME2,NAME3,NAME4,DESCRIPTION,LEVEL,&VARNAME,INIT);
**/

/**
//该函数的作用即是把指向Parameter对象的指针转换为ParPtr类型(通过const引用),然后把该对象(ParPtr)insert进我们的局部静态变量ParSet中,最后返回init值。
template<class T>const T&addGlobalParameter(const char *name,const char *description,int level,T*adr,const T&init)
{
  *adr=init; //这里是通过adr把它所指向的变量值修改，对应上面的宏定义语句，该变量就是VARNAME，即此处相当于VARNAME=init
  //ParameterChangedFlag一个bool型变量，bool ParameterChangedFlag=0;该定义在Parameter.cpp文件中，而Parameter.h头文件中是extern bool P...
  getGlobalParSet().insert(new Parameter<T>(name,ParameterChangedFlag,description,*adr,level));对于多name的情况，这里顺延，分别传入不同name
  return init; //最后这一条语句让该函数在处理返回值上显得前面有点多此一举的样子，但！实则不然，我们先关注Parameter<T>的构造函数，它比_Parameter
               //多接受一个参数T&_t,注意这里是传递引用！所以我们在该new语句中用*adr(对应我们的VARNAME全局变量)必有深意，不能简单传入init(常量)。
	       //Attention!我们new的Parameter对象的成员T *t对应的是用来初始化该对象的全局变量！！！，即t是指向我们的全局变量的指针！！！！
}
**/

/**
ParSet&getGlobalParSet()
{
  static ParSet x;
  return x;
}
**/

/**
class ParSet : public set<ParPtr>
{
 public:
  void insert(const ParPtr&x)
    {
      if( count(x)!=0 )
	cerr << "ERROR: element " << x->getString() << " already inserted.\n";
      set<ParPtr>::insert(x);
    }
};
**/

/**
const int PARLEV_ITER=1;
const int PARLEV_OPTHEUR=2;
const int PARLEV_OUTPUT=3;
const int PARLEV_SMOOTH=4;
const int PARLEV_EM=5;
const int PARLEV_MODELS=6;
const int PARLEV_SPECIAL=7;
const int PARLEV_INPUT=8
还有本main.cpp文件中前面的
#define ITER_M2 0
#define ITER_MH 5
**/
               // type,variable         ,name               ,name2                  ,name3,description                      ,level       ,init(初始值)
GLOBAL_PARAMETER3(int,Model1_Iterations,"Model1_Iterations","NO. ITERATIONS MODEL 1","m1","number of iterations for Model 1",PARLEV_ITER,5);
GLOBAL_PARAMETER3(int,Model2_Iterations,"Model2_Iterations","NO. ITERATIONS MODEL 2","m2","number of iterations for Model 2",PARLEV_ITER,ITER_M2);
GLOBAL_PARAMETER3(int,HMM_Iterations,"HMM_Iterations","mh","number of iterations for HMM alignment model","mh",              PARLEV_ITER,ITER_MH);
GLOBAL_PARAMETER3(int,Model3_Iterations,"Model3_Iterations","NO. ITERATIONS MODEL 3","m3","number of iterations for Model 3",PARLEV_ITER,5);
GLOBAL_PARAMETER3(int,Model4_Iterations,"Model4_Iterations","NO. ITERATIONS MODEL 4","m4","number of iterations for Model 4",PARLEV_ITER,5);
GLOBAL_PARAMETER3(int,Model5_Iterations,"Model5_Iterations","NO. ITERATIONS MODEL 5","m5","number of iterations for Model 5",PARLEV_ITER,0);
GLOBAL_PARAMETER3(int,Model6_Iterations,"Model6_Iterations","NO. ITERATIONS MODEL 6","m6","number of iterations for Model 6",PARLEV_ITER,0);


GLOBAL_PARAMETER(float, PROB_SMOOTH,"probSmooth","probability smoothing (floor) value ",PARLEV_OPTHEUR,1e-7);
GLOBAL_PARAMETER(float, MINCOUNTINCREASE,"minCountIncrease","minimal count increase",PARLEV_OPTHEUR,1e-7);

GLOBAL_PARAMETER2(int,Transfer_Dump_Freq,"TRANSFER DUMP FREQUENCY","t2to3","output: dump of transfer from Model 2 to 3",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER2(bool,Verbose,"verbose","v","0: not verbose; 1: verbose",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER(bool,Log,"log","0: no logfile; 1: logfile",PARLEV_OUTPUT,0);


GLOBAL_PARAMETER(double,P0,"p0","fixed value for parameter p_0 in IBM-3/4 (if negative then it is determined in training)",PARLEV_EM,-1.0);
GLOBAL_PARAMETER(double,M5P0,"m5p0","fixed value for parameter p_0 in IBM-5 (if negative then it is determined in training)",PARLEV_EM,-1.0);
GLOBAL_PARAMETER3(bool,Peg,"pegging","p","DO PEGGING? (Y/N)","0: no pegging; 1: do pegging",PARLEV_EM,0);

GLOBAL_PARAMETER(short,OldADBACKOFF,"adbackoff","",-1,0);
GLOBAL_PARAMETER2(unsigned int,MAX_SENTENCE_LENGTH,"ml","MAX SENTENCE LENGTH","maximum sentence length",0,MAX_SENTENCE_LENGTH_ALLOWED);
//const unsigned int MAX_SENTENCE_LENGTH_ALLOWED=101;
//所这条语句相当于创建了一个全局变量MAX_SENTENCE_LENGTH，赋值为MAX_SENTENCE_LENGTH_ALLOWED

GLOBAL_PARAMETER(short, DeficientDistortionForEmptyWord,"DeficientDistortionForEmptyWord","0: IBM-3/IBM-4 as described in (Brown et al. 1993); 1: distortion model of empty word is deficient; 2: distoriton model of empty word is deficient (differently); setting this parameter also helps to avoid that during IBM-3 and IBM-4 training too many words are aligned with the empty word",PARLEV_MODELS,0);
short OutputInAachenFormat=0;
bool Transfer=TRANSFER;
bool Transfer2to3=0;
short NoEmptyWord=0;
bool FEWDUMPS=0;
//下面是我们新增的全局变量
bool newflag=0;
//结束
GLOBAL_PARAMETER(bool,ONLYALDUMPS,"ONLYALDUMPS","1: do not write any files",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER(short,CompactAlignmentFormat,"CompactAlignmentFormat","0: detailled alignment format, 1: compact alignment format ",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER2(bool,NODUMPS,"NODUMPS","NO FILE DUMPS? (Y/N)","1: do not write any files",PARLEV_OUTPUT,0);

GLOBAL_PARAMETER(WordIndex,MAX_FERTILITY,"MAX_FERTILITY","maximal fertility for fertility models",PARLEV_EM,10);

Vector<map< pair<int,int>,char > > ReferenceAlignment;

//There we define some global parameters directly! one bool type, others are string.
bool useDict = false;
string CoocurrenceFile;
string Prefix, LogFilename, OPath, Usage, 
  SourceVocabFilename, TargetVocabFilename, CorpusFilename, 
  TestCorpusFilename, t_Filename, a_Filename, p0_Filename, d_Filename, 
  n_Filename, dictionary_Filename;

ofstream logmsg ;
const string str2Num(int n){
  string number = "";
  do{
    number.insert((size_t)0, 1, (char)(n % 10 + '0'));
  } while((n /= 10) > 0);
  return(number) ;
}


double LAMBDA=1.09;
sentenceHandler *testCorpus=0,*corpus=0;
Perplexity trainPerp, testPerp, trainViterbiPerp, testViterbiPerp ;

string ReadTablePrefix;


void printGIZAPars(ostream&out)
{
  out << "general parameters:\n"
         "-------------------\n";
  printPars(out,getGlobalParSet(),0);
  out << '\n';

  out << "No. of iterations:\n-"
         "------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_ITER);
  out << '\n';

  out << "parameter for various heuristics in GIZA++ for efficient training:\n"
         "------------------------------------------------------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_OPTHEUR);
  out << '\n';

  out << "parameters for describing the type and amount of output:\n"
         "-----------------------------------------------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_OUTPUT);
  out << '\n';

  out << "parameters describing input files:\n"
         "----------------------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_INPUT);
  out << '\n';

  out << "smoothing parameters:\n"
         "---------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_SMOOTH);
  out << '\n';

  out << "parameters modifying the models:\n"
         "--------------------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_MODELS);
  out << '\n';

  out << "parameters modifying the EM-algorithm:\n"
         "--------------------------------------\n";
  printPars(out,getGlobalParSet(),PARLEV_EM);
  out << '\n';
}

const char*stripPath(const char*fullpath)
  // strip the path info from the file name 
{
  const char *ptr = fullpath + strlen(fullpath) - 1 ;
  while(ptr && ptr > fullpath && *ptr != '/'){ptr--;}
  if( *ptr=='/' )
    return(ptr+1);
  else
    return ptr;
}


void printDecoderConfigFile()
{
  string decoder_config_file = Prefix + ".Decoder.config" ;
  cerr << "writing decoder configuration file to " <<  decoder_config_file.c_str() <<'\n';
  ofstream decoder(decoder_config_file.c_str());
  if(!decoder){
    cerr << "\nCannot write to " << decoder_config_file <<'\n';
    exit(1);
  }
  decoder << "# Template for Configuration File for the Rewrite Decoder\n# Syntax:\n" 
	  << "#         <Variable> = <value>\n#         '#' is the comment character\n"
	  << "#================================================================\n"
	  << "#================================================================\n"
	  << "# LANGUAGE MODEL FILE\n# The full path and file name of the language model file:\n";
  decoder << "LanguageModelFile =\n";
  decoder << "#================================================================\n"
	  << "#================================================================\n"
	  << "# TRANSLATION MODEL FILES\n# The directory where the translation model tables as created\n"
	  << "# by Giza are located:\n#\n"
	  << "# Notes: - All translation model \"source\" files are assumed to be in\n"
	  << "#          TM_RawDataDir, the binaries will be put in TM_BinDataDir\n"
	  << "#\n#        - Attention: RELATIVE PATH NAMES DO NOT WORK!!!\n"
	  << "#\n#        - Absolute paths (file name starts with /) will override\n"
	  << "#          the default directory.\n\n";
  // strip file prefix info and leave only the path name in Prefix
  string path = Prefix.substr(0, Prefix.find_last_of("/")+1);
  if( path=="" )
    path=".";
  decoder << "TM_RawDataDir = " << path << '\n';
  decoder << "TM_BinDataDir = " << path << '\n' << '\n';
  decoder << "# file names of the TM tables\n# Notes:\n"
	  << "# 1. TTable and InversTTable are expected to use word IDs not\n"
	  << "#    strings (Giza produces both, whereby the *.actual.* files\n"
	  << "#    use strings and are THE WRONG CHOICE.\n"
	  << "# 2. FZeroWords, on the other hand, is a simple list of strings\n"
	  << "#    with one word per line. This file is typically edited\n"
	  << "#    manually. Hoeever, this one listed here is generated by GIZA\n\n";
  
  int lastmodel;
  if (Model5_Iterations>0)
    lastmodel = 5 ;
  else if (Model4_Iterations>0)
    lastmodel = 4 ;
  else if (Model3_Iterations>0)
    lastmodel = 3 ;
  else if (Model2_Iterations>0)
    lastmodel = 2 ;
  else lastmodel = 1 ;
  string lastModelName = str2Num(lastmodel);
  string p=Prefix + ".t" + /*lastModelName*/"3" +".final";
  decoder << "TTable = " << stripPath(p.c_str()) << '\n';
  p = Prefix + ".ti.final" ;
  decoder << "InverseTTable = " << stripPath(p.c_str()) << '\n';
  p=Prefix + ".n" + /*lastModelName*/"3" + ".final";
  decoder << "NTable = " << stripPath(p.c_str())  << '\n';
  p=Prefix + ".d" + /*lastModelName*/"3" + ".final";
  decoder << "D3Table = " << stripPath(p.c_str())  << '\n';
  p=Prefix + ".D4.final";
  decoder << "D4Table = " << stripPath(p.c_str()) << '\n';
  p=Prefix + ".p0_"+ /*lastModelName*/"3" + ".final";
  decoder << "PZero = " << stripPath(p.c_str()) << '\n';
  decoder << "Source.vcb = " << SourceVocabFilename << '\n';
  decoder << "Target.vcb = " << TargetVocabFilename << '\n';
  //  decoder << "Source.classes = " << SourceVocabFilename + ".classes" << '\n';
  //  decoder << "Target.classes = " << TargetVocabFilename + ".classes" <<'\n';
  decoder << "Source.classes = " << SourceVocabFilename+".classes" << '\n';
  decoder << "Target.classes = " << TargetVocabFilename + ".classes" <<'\n';
  p=Prefix + ".fe0_"+ /*lastModelName*/"3" + ".final";
  decoder << "FZeroWords       = " <<stripPath(p.c_str()) << '\n' ;

  /*  decoder << "# Translation Parameters\n"
      << "# Note: TranslationModel and LanguageModelMode must have NUMBERS as\n"
      << "# values, not words\n"
      << "# CORRECT: LanguageModelMode = 2\n"
      << "# WRONG:   LanguageModelMode = bigrams # WRONG, WRONG, WRONG!!!\n";
      decoder << "TMWeight          = 0.6 # weight of TM for calculating alignment probability\n";
      decoder << "TranslationModel  = "<<lastmodel<<"   # which model to use (3 or 4)\n";
      decoder << "LanguageModelMode = 2   # (2 (bigrams) or 3 (trigrams)\n\n";
      decoder << "# Output Options\n"
      << "TellWhatYouAreDoing = TRUE # print diagnostic messages to stderr\n"
      << "PrintOriginal       = TRUE # repeat original sentence in the output\n"
      << "TopTranslations     = 3    # number of n best translations to be returned\n"
      << "PrintProbabilities  = TRUE # give the probabilities for the translations\n\n";
      
      decoder << "# LOGGING OPTIONS\n"
      << "LogFile = - # empty means: no log, dash means: STDOUT\n"
      << "LogLM = true # log language model lookups\n"
      << "LogTM = true # log translation model lookups\n";
      */
}


void printAllTables(vcbList& eTrainVcbList, vcbList& eTestVcbList,
		    vcbList& fTrainVcbList, vcbList& fTestVcbList, model1& m1)
{
  cerr << "writing Final tables to Disk \n";
  string t_inv_file = Prefix + ".ti.final" ;
  if( !FEWDUMPS)
    m1.getTTable().printProbTableInverse(t_inv_file.c_str(), m1.getEnglishVocabList(), 
					 m1.getFrenchVocabList(), 
					 m1.getETotalWCount(), 
					 m1.getFTotalWCount());
  t_inv_file = Prefix + ".actual.ti.final" ;
  if( !FEWDUMPS )
    m1.getTTable().printProbTableInverse(t_inv_file.c_str(), 
					 eTrainVcbList.getVocabList(), 
					 fTrainVcbList.getVocabList(), 
					 m1.getETotalWCount(), 
					 m1.getFTotalWCount(), true);
  
  string perp_filename = Prefix + ".perp" ;
  ofstream of_perp(perp_filename.c_str());
  
  cout << "Writing PERPLEXITY report to: " << perp_filename << '\n';
  if(!of_perp){
    cerr << "\nERROR: Cannot write to " << perp_filename <<'\n';
    exit(1);
  }
  
  if (testCorpus)
    generatePerplexityReport(trainPerp, testPerp, trainViterbiPerp, 
			     testViterbiPerp, of_perp, (*corpus).getTotalNoPairs1(), 
			     (*testCorpus).getTotalNoPairs1(),
			     true);
  else 
    generatePerplexityReport(trainPerp, testPerp, trainViterbiPerp, testViterbiPerp, 
			     of_perp, (*corpus).getTotalNoPairs1(), 0, true);
  
  string eTrainVcbFile = Prefix + ".trn.src.vcb" ;
  ofstream of_eTrainVcb(eTrainVcbFile.c_str());
  cout << "Writing source vocabulary list to : " << eTrainVcbFile << '\n';
  if(!of_eTrainVcb){
    cerr << "\nERROR: Cannot write to " << eTrainVcbFile <<'\n';
    exit(1);
  }
  eTrainVcbList.printVocabList(of_eTrainVcb) ;
  
  string fTrainVcbFile = Prefix + ".trn.trg.vcb" ;
  ofstream of_fTrainVcb(fTrainVcbFile.c_str());
  cout << "Writing source vocabulary list to : " << fTrainVcbFile << '\n';
  if(!of_fTrainVcb){
    cerr << "\nERROR: Cannot write to " << fTrainVcbFile <<'\n';
    exit(1);
  }
  fTrainVcbList.printVocabList(of_fTrainVcb) ;
  
  //print test vocabulary list 
  
  string eTestVcbFile = Prefix + ".tst.src.vcb" ;
  ofstream of_eTestVcb(eTestVcbFile.c_str());
  cout << "Writing source vocabulary list to : " << eTestVcbFile << '\n';
  if(!of_eTestVcb){
    cerr << "\nERROR: Cannot write to " << eTestVcbFile <<'\n';
    exit(1);
  }
  eTestVcbList.printVocabList(of_eTestVcb) ;
  
  string fTestVcbFile = Prefix + ".tst.trg.vcb" ;
  ofstream of_fTestVcb(fTestVcbFile.c_str());
  cout << "Writing source vocabulary list to : " << fTestVcbFile << '\n';
  if(!of_fTestVcb){
    cerr << "\nERROR: Cannot write to " << fTestVcbFile <<'\n';
    exit(1);
  }
  fTestVcbList.printVocabList(of_fTestVcb) ;
  printDecoderConfigFile();
  if (testCorpus)
    printOverlapReport(m1.getTTable(), *testCorpus, eTrainVcbList, 
		       fTrainVcbList, eTestVcbList, fTestVcbList);
  
}

bool readNextSent(istream&is,map< pair<int,int>,char >&s,int&number)
{
  string x;
  if( !(is >> x) ) return 0;
  if( x=="SENT:" ) is >> x;
  int n=atoi(x.c_str());
  if( number==-1 )
    number=n;
  else
    if( number!=n )
      {
	cerr << "ERROR: readNextSent: DIFFERENT NUMBERS: " << number << " " << n << '\n';
	return 0;
      }
  int nS,nP,nO;
  nS=nP=nO=0;
  while( is >> x )
    {
      if( x=="SENT:" )
	return 1;
      int n1,n2;
      is >> n1 >> n2;
      map< pair<int,int>,char >::const_iterator i=s.find(pair<int,int>(n1,n2));
      if( i==s.end()||i->second=='P' )
	s[pair<int,int>(n1,n2)]=x[0];
      massert(x[0]=='S'||x[0]=='P');
      nS+= (x[0]=='S');
      nP+= (x[0]=='P');
      nO+= (!(x[0]=='S'||x[0]=='P'));
    }
  return 1;
}

bool emptySent(map< pair<int,int>,char >&x)
{
  x = map< pair<int,int>,char >();
  return 1;
}

void ReadAlignment(const string&x,Vector<map< pair<int,int>,char > >&a)
{
  ifstream infile(x.c_str());
  a.clear();
  map< pair<int,int>,char >sent;
  int number=0;
  while( emptySent(sent) && (readNextSent(infile,sent,number)) )
    {
      if( int(a.size())!=number )
	cerr << "ERROR: ReadAlignment: " << a.size() << " " << number << '\n';
      a.push_back(sent);
      number++;
    }
  cout << "Read: " << a.size() << " sentences in reference alignment." << '\n';
}
    

void initGlobals(void)
{
  NODUMPS = false ;
  Prefix = Get_File_Spec();
  LogFilename= Prefix + ".log";
  MAX_SENTENCE_LENGTH = MAX_SENTENCE_LENGTH_ALLOWED ;
}

void convert(const map< pair<int,int>,char >&reference,alignment&x)
{
  int l=x.get_l();
  int m=x.get_m();
  for(map< pair<int,int>,char >::const_iterator i=reference.begin();i!=reference.end();++i)
    {
      if( i->first.first+1>int(m) )
	{
	  cerr << "ERROR m to big: " << i->first.first << " " << i->first.second+1 << " " << l << " " << m << " is wrong.\n";
	  continue;
	}
      if( i->first.second+1>int(l) )
	{
	  cerr << "ERROR l to big: " << i->first.first << " " << i->first.second+1 << " " << l << " " << m << " is wrong.\n";
	  continue;
	}
      if( x(i->first.first+1)!=0 )
	cerr << "ERROR: position " << i->first.first+1 << " already set\n";
      x.set(i->first.first+1,i->first.second+1);
    }
}
double ErrorsInAlignment(const map< pair<int,int>,char >&reference,const Vector<WordIndex>&test,int l,int&missing,int&toomuch,int&eventsMissing,int&eventsToomuch,int pair_no)
{
  int err=0;
  for(unsigned int j=1;j<test.size();j++)
    {
      if( test[j]>0 )
	{
	  map< pair<int,int>,char >::const_iterator i=reference.find(make_pair(test[j]-1,j-1));
	  if( i==reference.end() )
	    {
	      toomuch++;
	      err++;
	    }
	  else
	    if( !(i->second=='S' || i->second=='P'))
	      cerr << "ERROR: wrong symbol in reference alignment '" << i->second << ' ' << int(i->second) << " no:" << pair_no<< "'\n";
	  eventsToomuch++;
	}
    }
  for(map< pair<int,int>,char >::const_iterator i=reference.begin();i!=reference.end();++i)
    {
      if( i->second=='S' )
	{
	  unsigned int J=i->first.second+1;
	  unsigned int I=i->first.first+1;
	  if( int(J)>=int(test.size())||int(I)>int(l)||int(J)<1||int(I)<1 )
	    cerr << "ERROR: alignment outside of range in reference alignment" << J << " " << test.size() << " (" << I << " " << l << ") no:" << pair_no << '\n';
	  else
	    {
	      if(test[J]!=I)
		{
		  missing++;
		  err++;
		}
	    }
	  eventsMissing++;
	}
    }
  if( Verbose )
    cout << err << " errors in sentence\n";
  if( eventsToomuch+eventsMissing )
    return (toomuch+missing)/(eventsToomuch+eventsMissing);
  else
    return 1.0;
}
/*
class vcbList{
 private:
  Vector<WordEntry> list ;
  map<string,int> s2i;
  double total;
  WordIndex noUniqueTokens ;
  WordIndex noUniqueTokensInCorpus ;
  const char* fname ;
 public:
  .....
*/

vcbList *globeTrainVcbList,*globfTrainVcbList;


//我们自己创建的新的StartTesting函数
double StartTesting(int &result)
{
  double errors=0.0;
  vcbList eTrainVcbList, fTrainVcbList;
  globeTrainVcbList=&eTrainVcbList;
  globfTrainVcbList=&fTrainVcbList;
	
  eTrainVcbList.setName(SourceVocabFilename.c_str());
  fTrainVcbList.setName(TargetVocabFilename.c_str());
  eTrainVcbList.readVocabList();
  fTrainVcbList.readVocabList();
	
  vcbList eTestVcbList(eTrainVcbList) ; //here we know that no need to give testVcb,we use trainVcb to initialize them.
  vcbList fTestVcbList(fTrainVcbList) ;
	
  corpus = new sentenceHandler(CorpusFilename.c_str(), &eTrainVcbList, &fTrainVcbList);
  if (TestCorpusFilename == "NONE")
    TestCorpusFilename = "";

  if (TestCorpusFilename != "")
      testCorpus= new sentenceHandler(TestCorpusFilename.c_str(), &eTestVcbList, &fTestVcbList);
  useDict=0;
  dictionary = new Dictionary("");
  int minIter=0;
  //下面的宏判断我还不确定在test阶段要怎么修改，暂时先不动它
  /*
  #ifdef BINARY_SEARCH_FOR_TTABLE
  if( CoocurrenceFile.length()==0 )
    {
      cerr << "ERROR: NO COOCURRENCE FILE GIVEN!\n";
      abort();
    }
  //ifstream coocs(CoocurrenceFile.c_str());
  tmodel<COUNT, PROB> tTable(CoocurrenceFile);
  #else
  tmodel<COUNT, PROB> tTable;
  #endif
  */
   
   model1 m1(CorpusFilename.c_str(), eTrainVcbList, fTrainVcbList,tTable,trainPerp, 
	    *corpus,&testPerp, testCorpus, //这里的testPerp是前面定义的全局变量所以它必不为NULL,而testCorpus则取决于我们在./GIZA++时是否传入-tc参数。
	    trainViterbiPerp, &testViterbiPerp);//trainViterbiPerp,testViterbiPerp同样是前面定义的未初始化的全局变量
   amodel<PROB>  aTable(false); // typedef float PROB ;
   amodel<COUNT> aCountTable(false); //typedef float COUNT ;
   model2 m2(m1,aTable,aCountTable);
   //对m2进行load_table操作
   model1 *p1=&m2; 
   p1->load_table("ttable.file"); //这里故意使用非virtual的函数，其实貌似直接m1.load_table(..)也可以
   m2.load_table("atable.file");
   //load结束
   hmm h(m2);
   model3 m3(m2);
	
   if( HMM_Iterations>0 )
       m3.setHMM(&h);
   if(Model3_Iterations > 0 || Model4_Iterations > 0 || Model5_Iterations || Model6_Iterations)
      {
	 minIter=m3.viterbi_test(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations);
	 errors=m3.errorsAL();
      }
   result=minIter;
   return errors;
}



double StartTraining(int&result)//这里result默认是-1
{ 
  double errors=0.0;
  vcbList eTrainVcbList, fTrainVcbList;
  globeTrainVcbList=&eTrainVcbList;
  globfTrainVcbList=&fTrainVcbList;


  string repFilename = Prefix + ".gizacfg" ;
  ofstream of2(repFilename.c_str());
  writeParameters(of2,getGlobalParSet(),-1) ;

  cout << "reading vocabulary files \n";
  eTrainVcbList.setName(SourceVocabFilename.c_str());
  fTrainVcbList.setName(TargetVocabFilename.c_str());
  eTrainVcbList.readVocabList();
  fTrainVcbList.readVocabList();
  cout << "Source vocabulary list has " << eTrainVcbList.uniqTokens() << " unique tokens \n";
  cout << "Target vocabulary list has " << fTrainVcbList.uniqTokens() << " unique tokens \n";
  
  vcbList eTestVcbList(eTrainVcbList) ; //here we know that no need to give testVcb,we use trainVcb to initialize them.
  vcbList fTestVcbList(fTrainVcbList) ;
  
  corpus = new sentenceHandler(CorpusFilename.c_str(), &eTrainVcbList, &fTrainVcbList);

  if (TestCorpusFilename == "NONE")
    TestCorpusFilename = "";

  if (TestCorpusFilename != ""){
    cout << "Test corpus will be read from: " << TestCorpusFilename << '\n';
      testCorpus= new sentenceHandler(TestCorpusFilename.c_str(), 
						       &eTestVcbList, &fTestVcbList);
      cout << " Test total # sentence pairs : " <<(*testCorpus).getTotalNoPairs1()<<" weighted:"<<(*testCorpus).getTotalNoPairs2() <<'\n';

      cout << "Size of the source portion of test corpus: " << eTestVcbList.totalVocab() << " tokens\n";
      cout << "Size of the target portion of test corpus: " << fTestVcbList.totalVocab() << " tokens \n";
      cout << "In source portion of the test corpus, only " << eTestVcbList.uniqTokensInCorpus() << " unique tokens appeared\n";
      cout << "In target portion of the test corpus, only " << fTestVcbList.uniqTokensInCorpus() << " unique tokens appeared\n";
      cout << "ratio (target/source) : " << double(fTestVcbList.totalVocab()) /
	eTestVcbList.totalVocab() << '\n';
  }
  
  cout << " Train total # sentence pairs (weighted): " << corpus->getTotalNoPairs2() << '\n';
  cout << "Size of source portion of the training corpus: " << eTrainVcbList.totalVocab()-corpus->getTotalNoPairs2() << " tokens\n";
  cout << "Size of the target portion of the training corpus: " << fTrainVcbList.totalVocab() << " tokens \n";
  cout << "In source portion of the training corpus, only " << eTrainVcbList.uniqTokensInCorpus() << " unique tokens appeared\n";
  cout << "In target portion of the training corpus, only " << fTrainVcbList.uniqTokensInCorpus() << " unique tokens appeared\n";
  cout << "lambda for PP calculation in IBM-1,IBM-2,HMM:= " << double(fTrainVcbList.totalVocab()) << "/(" << eTrainVcbList.totalVocab() << "-" << corpus->getTotalNoPairs2() << ")=";
  LAMBDA = double(fTrainVcbList.totalVocab()) / (eTrainVcbList.totalVocab()-corpus->getTotalNoPairs2());
  cout << "= " << LAMBDA << '\n';
  // load dictionary
/*
class Dictionary{
 private:
  Vector<int> pairs[2];
  int currval;
  int currindexmin;
  int currindexmax;
  bool dead;
 public:
  Dictionary(const char *);
  bool indict(int, int);
};
*/
  Dictionary *dictionary;  
//  getGlobalParSet().insert(new Parameter<string>("d",ParameterChangedFlag,"dictionary file name",dictionary_Filename,PARLEV_INPUT)); #const int PARLEV_INPUT=8
//  getGlobalParSet().insert(new Parameter<string>("DICTIONARY",ParameterChangedFlag,"dictionary file name",dictionary_Filename,-1));
//这里的dictionary_Filename是我们定义的全局变量，它接受我们调用./GIZA++ ...的参数作为值，一般我们不适用dictionary文件，所以它为空，所以useDict为false
  useDict = !dictionary_Filename.empty();
  if (useDict) dictionary = new Dictionary(dictionary_Filename.c_str());
  else dictionary = new Dictionary("");
  int minIter=0;
#ifdef BINARY_SEARCH_FOR_TTABLE
  if( CoocurrenceFile.length()==0 )
    {
      cerr << "ERROR: NO COOCURRENCE FILE GIVEN!\n";
      abort();
    }
  //ifstream coocs(CoocurrenceFile.c_str());
  tmodel<COUNT, PROB> tTable(CoocurrenceFile);
#else
  tmodel<COUNT, PROB> tTable;
#endif
	
	
  // 下面这里是我们把训练所需的文件传入模型，然后我们一级一级的使用简单的模型去初始化复杂模型
  // Perplexity trainPerp, testPerp, trainViterbiPerp, testViterbiPerp;
  // 下面参数中的tTable是在之前定义的：tmodel<COUNT, PROB> tTable; 还没有初始化。当然如果我们有BINARY_SEARCH_FOR_TTABLE的话，
  // 那么就变成了tmodel<COUNT, PROB> tTable(CoocurrenceFile); 定义且初始化了。
   model1 m1(CorpusFilename.c_str(), eTrainVcbList, fTrainVcbList,tTable,trainPerp, 
	    *corpus,&testPerp, testCorpus, //这里的testPerp是前面定义的全局变量所以它必不为NULL,而testCorpus则取决于我们在./GIZA++时是否传入-tc参数。
	    trainViterbiPerp, &testViterbiPerp);//trainViterbiPerp,testViterbiPerp同样是前面定义的未初始化的全局变量
   //这里传入的是*corpus和testcorpus,一个sentenceHandler一个是指向sentenceHandler的指针。
   amodel<PROB>  aTable(false); // typedef float PROB ;
   amodel<COUNT> aCountTable(false); //typedef float COUNT ;
   model2 m2(m1,aTable,aCountTable);
   hmm h(m2);
   model3 m3(m2); 
	
	
   //这个if不用看
   if(ReadTablePrefix.length() ) //这里注意默认我们并不使用ReadTablePrefix参数，所以该全局变量长度为0，所以默认情况下不进入这里的逻辑
	                         //这里的逻辑和else中的逻辑大同小异
     {
       string number = "final";
       string tfile,afilennfile,dfile,d4file,p0file,afile,nfile; //d5file
       tfile = ReadTablePrefix + ".t3." + number ;
       afile = ReadTablePrefix + ".a3." + number ;
       nfile = ReadTablePrefix + ".n3." + number ;
       dfile = ReadTablePrefix + ".d3." + number ;
       d4file = ReadTablePrefix + ".d4." + number ;
       //d5file = ReadTablePrefix + ".d5." + number ;
       p0file = ReadTablePrefix + ".p0_3." + number ;
       tTable.readProbTable(tfile.c_str());
       aTable.readTable(afile.c_str());
       m3.dTable.readTable(dfile.c_str());
       m3.nTable.readNTable(nfile.c_str());
       sentPair sent ;
       double p0;
       ifstream p0f(p0file.c_str());
       p0f >> p0;
       d4model d4m(MAX_SENTENCE_LENGTH);
       d4m.makeWordClasses(m1.Elist,m1.Flist,SourceVocabFilename+".classes",TargetVocabFilename+".classes");
       d4m.readProbTable(d4file.c_str());
       //d5model d5m(d4m);
       //d5m.makeWordClasses(m1.Elist,m1.Flist,SourceVocabFilename+".classes",TargetVocabFilename+".classes");
       //d5m.readProbTable(d5file.c_str());
       makeSetCommand("model4smoothfactor","0.0",getGlobalParSet(),2);
       //makeSetCommand("model5smoothfactor","0.0",getGlobalParSet(),2);
       if( corpus||testCorpus )
	 {
	   sentenceHandler *x=corpus;
	   if(x==0)
	     x=testCorpus;
	   cout << "Text corpus exists.\n";
	   x->rewind();
	   while(x&&x->getNextSentence(sent)){
	     Vector<WordIndex>& es = sent.eSent;
	     Vector<WordIndex>& fs = sent.fSent;
	     int l=es.size()-1;
	     int m=fs.size()-1;
	     transpair_model4 tm4(es,fs,m1.tTable,m2.aTable,m3.dTable,m3.nTable,1-p0,p0,&d4m);
	     alignment al(l,m);
	     cout << "I use the alignment " << sent.sentenceNo-1 << '\n';
	     //convert(ReferenceAlignment[sent.sentenceNo-1],al);
	     transpair_model3 tm3(es,fs,m1.tTable,m2.aTable,m3.dTable,m3.nTable,1-p0,p0,0);
	     double p=tm3.prob_of_target_and_alignment_given_source(al,1);
	     cout << "Sentence " << sent.sentenceNo << " has IBM-3 prob " << p << '\n';
	     p=tm4.prob_of_target_and_alignment_given_source(al,3,1);
	     cout << "Sentence " << sent.sentenceNo << " has IBM-4 prob " << p << '\n';
	     //transpair_model5 tm5(es,fs,m1.tTable,m2.aTable,m3.dTable,m3.nTable,1-p0,p0,&d5m);
	     //p=tm5.prob_of_target_and_alignment_given_source(al,3,1);
	     //cout << "Sentence " << sent.sentenceNo << " has IBM-5 prob " << p << '\n';
	   }
	 }
       else
	 {
	   cout << "No corpus exists.\n";
	 }
    }
   else  //下面的逻辑是StartTraining函数的核心部分,下面的逻辑很清楚，我们使用一写指标(比如Model1_Iterations即model1迭代次数)来判断是否执行对应的训练
	 /*
	    model1 m1(CorpusFilename.c_str(), eTrainVcbList, fTrainVcbList,tTable,trainPerp, 
	    *corpus,&testPerp, testCorpus, 
	    trainViterbiPerp, &testViterbiPerp); //这里传入的是*corpus和testcorpus,一个sentenceHandler一个是指向sentenceHandler的指针。
            amodel<PROB>  aTable(false);  // typedef float PROB ;
            amodel<COUNT> aCountTable(false); //typedef float COUNT ;
            model2 m2(m1,aTable,aCountTable);
            hmm h(m2);
            model3 m3(m2); 
	  */
     {
       // initialize model1
       bool seedModel1 = false ;
       if(Model1_Iterations > 0){
	 if (t_Filename != "NONE" && t_Filename != ""){  //这里的t_Filename我们只是创建了这个string变量，但没有初始化或者赋值，所以它必为空
         //所以这个if block并不进入，这里我们的t_Filename在main函数中也没有用到，很可能是这部分代码并没有完成
	   seedModel1 = true ;
	   m1.load_table(t_Filename.c_str());
	 }
	 minIter=m1.em_with_tricks(Model1_Iterations,seedModel1,*dictionary, useDict);
	 errors=m1.errorsAL();
       }
       
	 {
	   if(Model2_Iterations > 0){
	     m2.initialize_table_uniformly(*corpus);
	     minIter=m2.em_with_tricks(Model2_Iterations);
	     errors=m2.errorsAL();
	   }
	   if(HMM_Iterations > 0){
	     cout << "NOTE: I am doing iterations with the HMM model!\n";
	     h.makeWordClasses(m1.Elist,m1.Flist,SourceVocabFilename+".classes",TargetVocabFilename+".classes");
	     h.initialize_table_uniformly(*corpus);
	     minIter=h.em_with_tricks(HMM_Iterations);
	     errors=h.errorsAL();
	   }
	   //这里我添加了把ttable和atable打印到文件中的操作
	   {
	     h.print_ttables("ttable.file");
	     h.print_atables("atable.file");
	   }
	    	 

/*
short OutputInAachenFormat=0;
bool Transfer=TRANSFER;
bool Transfer2to3=0;
short NoEmptyWord=0;
bool FEWDUMPS=0;
*/
	   //这里由于Transfer2to3被初始化为0，且在该语句前没有改变值；HMM_Iterations则是我们的初值为ITER_MH(ITER_MH=5)的全局变量
	   //所以该if block不会进入
	   if(Transfer2to3||HMM_Iterations==0){
	     if( HMM_Iterations>0 )
	       cout << "WARNING: transfor is not needed, as results are overwritten bei transfer from HMM.\n";
	     string test_alignfile = Prefix +".tst.A2to3";
	     if (testCorpus)
	       m2.em_loop(testPerp, *testCorpus,Transfer_Dump_Freq==1&&!NODUMPS,test_alignfile.c_str(), testViterbiPerp, true);
	     if (testCorpus)
	       cout << "\nTransfer: TEST CROSS-ENTROPY " << testPerp.cross_entropy() << " PERPLEXITY " << testPerp.perplexity() << "\n\n";
	     if (Transfer == TRANSFER_SIMPLE)
	       m3.transferSimple(*corpus, Transfer_Dump_Freq==1&&!NODUMPS,trainPerp, trainViterbiPerp);
	     else  
	       m3.transfer(*corpus, Transfer_Dump_Freq==1&&!NODUMPS, trainPerp, trainViterbiPerp);
	     errors=m3.errorsAL();
	   }
	   
		 
		 
           //void setHMM(hmm*_h){h=_h;},在这里修改我们的model3的数据成员h(之前我们在构造model3对象时对于除基类model2以外都使用了默认参数，h被初始化为空指针)
	   if( HMM_Iterations>0 )
	     m3.setHMM(&h);
	   if(Model3_Iterations > 0 || Model4_Iterations > 0 || Model5_Iterations || Model6_Iterations
	      )
	     {
	       minIter=m3.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations);
	       errors=m3.errorsAL();
	     }
		 
		 
		 
	   if (FEWDUMPS||!NODUMPS) //由于FEWDUMPS和NODUMPS的值在执行到这里时都为0(实际上他们的初值都为为0,我们并没有改变它们的值)
		                   //所以必然会进入该if block
	     {
	       printAllTables(eTrainVcbList,eTestVcbList,fTrainVcbList,fTestVcbList,m1 );
	       //这里我们是把m1的em迭代的结果打印出来，打印到文件，这里共有后缀为:ti.final,actual.ti.final,prep,
	       //trn.src.vcb,trn.trg.vcb,tst.src.vcb,tst.trg.vcb的文件
	     }
	 }
     }
   result=minIter;
   return errors;
}

int main(int argc, char* argv[])
{
/*
这些是我们前面直接定义的全局变量(不是通过那个带参宏定义)
bool useDict = false;
string CoocurrenceFile;
string Prefix, LogFilename, OPath, Usage, 
  SourceVocabFilename, TargetVocabFilename, CorpusFilename, 
  TestCorpusFilename, t_Filename, a_Filename, p0_Filename, d_Filename, 
  n_Filename, dictionary_Filename;
double LAMBDA=1.09;
sentenceHandler *testCorpus=0,*corpus=0;
Perplexity trainPerp, testPerp, trainViterbiPerp, testViterbiPerp ;
string ReadTablePrefix;
*/

//我们下面没有采用一开始的宏定义方法，主要是我们对于这些全局变量没有初始值，而且我们下面的全局变量都是string类型的。更深入些讲前面的全局变量是
//比如模型迭代次数，平滑参数之类的整型值，下面则是对应为命令行参数，文件名之类的string类型值。
#ifdef BINARY_SEARCH_FOR_TTABLE
  getGlobalParSet().insert(new Parameter<string>("CoocurrenceFile",ParameterChangedFlag,"",CoocurrenceFile,PARLEV_SPECIAL));
#endif
  //下面这条是新加的
  getGlobalParSet().insert(new Parameter<bool>("NEW",ParameterChangedFlag,"flag for new parameter",newflag,-1));
  //结束
  getGlobalParSet().insert(new Parameter<string>("ReadTablePrefix",ParameterChangedFlag,"optimized",ReadTablePrefix,-1));
  getGlobalParSet().insert(new Parameter<string>("S",ParameterChangedFlag,"source vocabulary file name",SourceVocabFilename,PARLEV_INPUT));
  getGlobalParSet().insert(new Parameter<string>("SOURCE VOCABULARY FILE",ParameterChangedFlag,"source vocabulary file name",SourceVocabFilename,-1));
  getGlobalParSet().insert(new Parameter<string>("T",ParameterChangedFlag,"target vocabulary file name",TargetVocabFilename,PARLEV_INPUT));
  getGlobalParSet().insert(new Parameter<string>("TARGET VOCABULARY FILE",ParameterChangedFlag,"target vocabulary file name",TargetVocabFilename,-1));
  getGlobalParSet().insert(new Parameter<string>("C",ParameterChangedFlag,"training corpus file name",CorpusFilename,PARLEV_INPUT));
  getGlobalParSet().insert(new Parameter<string>("CORPUS FILE",ParameterChangedFlag,"training corpus file name",CorpusFilename,-1));
  getGlobalParSet().insert(new Parameter<string>("TC",ParameterChangedFlag,"test corpus file name",TestCorpusFilename,PARLEV_INPUT));
  getGlobalParSet().insert(new Parameter<string>("TEST CORPUS FILE",ParameterChangedFlag,"test corpus file name",TestCorpusFilename,-1));
  getGlobalParSet().insert(new Parameter<string>("d",ParameterChangedFlag,"dictionary file name",dictionary_Filename,PARLEV_INPUT));
  getGlobalParSet().insert(new Parameter<string>("DICTIONARY",ParameterChangedFlag,"dictionary file name",dictionary_Filename,-1));
  getGlobalParSet().insert(new Parameter<string>("l",ParameterChangedFlag,"log file name",LogFilename,PARLEV_OUTPUT));
  getGlobalParSet().insert(new Parameter<string>("LOG FILE",ParameterChangedFlag,"log file name",LogFilename,-1));

  getGlobalParSet().insert(new Parameter<string>("o",ParameterChangedFlag,"output file prefix",Prefix,PARLEV_OUTPUT));
  getGlobalParSet().insert(new Parameter<string>("OUTPUT FILE PREFIX",ParameterChangedFlag,"output file prefix",Prefix,-1));
  getGlobalParSet().insert(new Parameter<string>("OUTPUT PATH",ParameterChangedFlag,"output path",OPath,PARLEV_OUTPUT));

  time_t st1, fn;
  st1 = time(NULL);                    // starting time

  string temp(argv[0]);
  Usage = temp + " <config_file> [options]\n";
  if(argc < 2)
    {
      printHelp();    
      exit(1);
    }
  
  initGlobals() ;
/*
void initGlobals(void)
{
  NODUMPS = false ;
  Prefix = Get_File_Spec();
  LogFilename= Prefix + ".log";
  MAX_SENTENCE_LENGTH = MAX_SENTENCE_LENGTH_ALLOWED ;
}
*/
  parseArguments(argc, argv);
/*
void parseArguments(int argc, char *argv[])
{
  int arg = 1;
  //下面是一个提前的判断
  if(!strcmp(argv[1], "--h") || !strcmp(argv[1], "--help")){
    printHelp();
    exit(0);
  }
  if( argv[1][0]=='-' )
    arg=0;
  else
    parseConfigFile(argv[1]);   //这里对应的其实就是对应我们在使用./GIZA++ NAME.gizacfg 的形式
  //如果不走上面的else分支，这里arg=0，所以++arg为1，而我们的argc也包括执行文件本身的名字在内算一个参数，
  //所以这里的逻辑就是对每一个参数(除执行文件名外)，进行相应处理
  while(++arg<argc){ 
    if( strlen(argv[arg])>2 && argv[arg][0]=='-' && argv[arg][1]=='-' ) //这里对应的是--+的情况
      {
	if( !makeSetCommand(argv[arg]+1,"1",getGlobalParSet(),2)) 
	//这里argv[arg]是一个char *类型，给它加1即是是该指针向后移动移一位，这样就减少了一个'-',但我认为这里应该是+2，否则还是有一轮无用的检测
	  cerr << "WARNING: ignoring unrecognized option:  "<< argv[arg] << '\n' ;  
      }
    else if( arg+1<argc && !makeSetCommand(argv[arg],argv[arg+1],getGlobalParSet(),2)) //这里对应的是-command的情况
      cerr << "WARNING: ignoring unrecognized option:  "<< argv[arg] << '\n' ;  
    else
      {
	arg++; //这里对应的arg+1>=argc或makeSetCommand(argv[arg],argv[arg+1],getGlobalParSet(),2)返回1的情况，
	       //假如makeSetCommand(argv[arg],argv[arg+1],getGlobalParSet(),2)成功修改，那么比如 -c corpusname，我们希望跳过corpusname这个参数
      }
  }
  if( OPath.length() )
    OPath+="/";
  Prefix = (OPath + Prefix);
  LogFilename = (OPath + LogFilename);
  printGIZAPars(cout);
}
*/
	
/*
makeSetCommand的作用是用s1在我们的ParSet中去寻找(通过name成员匹配,全部匹配或者起始子串匹配)，如果找到，则用s2去修改对应Parameter对象的(*t)，
即对应全局变量的值，并返回1，否则返回0.
bool makeSetCommand(string s1,string s2,const ParSet&pars,int verb=1,int level= -1);
bool makeSetCommand(string _s1,string s2,const ParSet&parset,int verb,int level) //对于parseArgument中的makeSetCommand这里都是level=-1默认级别
                                                                                 //verb也都是传入2(>1)
{
  ParPtr anf;
  int anfset=0;
  string s1=simpleString(_s1);
  for(ParSet::const_iterator i=parset.begin();i!=parset.end();++i)
    {
      if( *(*i)==s1 ) //如果对应找到了,由下面的描述易知这里的有一步多余的simpleString操作。
      {
      //这里(*i)首先是解引用iterator，得到是ParPtr[typedef MP<_Parameter> ParPtr; //可以粗略的认为是指向_Parameter对象的“指针”对象]
      //然后再解引用*(*i)则是想当于得到_Parameter对象本身
      //这里对于*(*i)即_Parameter对象，它重载了‘==’运算符
      //bool operator==(const string&s)const
      //{ return name== simpleString(s); }
   
	  if( level==-1 || level==(*i)->getLevel() ) //int getLevel() const { return level;}
	    (*i)->setParameter(s2,verb);
	  else if(verb>1) //这里是如果level条件不满足，则再检查verb参数
	    cerr << "ERROR: Could not set: (A) " << s1 << " " << s2 << " " << level << " " << (*i)->getLevel() << endl;
	  return 1; //这里在parseArugment中的makeSetCommand的verb传入2，level默认为-1，则如果*(*i)==s1,返回1
	}
      else if( (*i)->getString().substr(0,s1.length())==s1 ) //如果*(*i)!=s1，但(*i)的子串有等于s1的，进入这一步
	{
	  anf=(*i);anfset++;
	}
    }
  //上面的for循环是在我们的ParSet中寻找我们的s1对应的参数(通过参数名字匹配)
  if(anfset==1)//如果在(*i)的子串中只匹配到了一次s1，那么也是同样的操作
    {
      if( level==-1 || level==anf->getLevel() )
	anf->setParameter(s2,verb);
      else if( verb>1 )
	cerr << "ERROR: Could not set: (B) " << s1 << " " << s2 << " " << level << " " << anf->getLevel() << endl;
      return 1;
    }
  if( anfset>1 )
    cerr << "ERROR: ambiguous parameter '" << s1 << "'.\n";
  if( anfset==0 )
    cerr << "ERROR: parameter '" << s1 << "' does not exist.\n";
  return 0;
}
*/
/*
经simpleString处理后的返回的string可以理解为是输入string的小写，去掉特殊字符(这里是除了a-z,0-9之外),这里的c[1]=0的意思是把每次的char数组变为一个char字符串，即末尾为空字符
string simpleString(const string s)
{
  string k;
  for(unsigned int i=0;i<s.length();++i)
    {
      char c[2];
      c[0]=tolower(s[i]);
      c[1]=0;
      if( (c[0]>='a'&&c[0]<='z')||(c[0]>='0'&&c[0]<='9') )
	k += c;
    }
  return k;
}
*/
  
  if (Log)
    logmsg.open(LogFilename.c_str(), ios::out);
  
  printGIZAPars(cout);
  int a=-1;
  double errors=0.0;
  if( OldADBACKOFF!=0 )
    cerr << "WARNING: Parameter -adBackOff does not exist further; use CompactADTable instead.\n";
  if( MAX_SENTENCE_LENGTH > MAX_SENTENCE_LENGTH_ALLOWED )
    cerr << "ERROR: MAX_SENTENCE_LENGTH is too big " << MAX_SENTENCE_LENGTH << " > " << MAX_SENTENCE_LENGTH_ALLOWED << '\n';
  if(newflag==0)
  {
    errors=StartTraining(a); //这里原来有一个缩进，很迷惑(如果你也熟悉python编程的话)，所以我把它去掉了
  }
  else
  {
    errors=StartTesting(a);
  }	
  fn = time(NULL);    // finish time
  cout << '\n' << "Entire Training took: " << difftime(fn, st1) << " seconds\n";
  cout << "Program Finished at: "<< ctime(&fn) << '\n';
  cout << "==========================================================\n";
  return 0;
}

