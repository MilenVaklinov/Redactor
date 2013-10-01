#include "consoleSourceCounter.h"

#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QTime>
#include <QVector>
#include <iostream>
#include <string>
#include <QDebug>

SourceCounter::SourceCounter(const QString &folderName)
{
    timer = new QTime;
    timer->start();

    initVariables();

    name_of_the_source = folderName;

    QDir dir(name_of_the_source);
    if(!dir.exists()) {
        qDebug() << "The foldert does not exist!";
        return;
    }

    found(name_of_the_source);

    printResult();
}

SourceCounter::~SourceCounter()
{
    delete filtersPtr;
    delete timer;
    delete myMap;
}

void SourceCounter::printResult()
{
    std::cout << std::string(70, '-') << std::endl;

    std::cout << "Language" << std::string(22, ' ')
              << "files" << std::string(5, ' ')
              << "blank" << std::string(5, ' ')
              << "comment" << std::string(3, ' ')
              << "code" << std::string(6, ' ') << std::endl;

    std::cout << std::string(70, '-') << std::endl;

    QMap<QString, QVector<int> >::const_iterator i;
    for(i = myMap->constBegin(); i != myMap->constEnd(); ++i) {
        std::cout << QString(i.key()).toLocal8Bit().constData()
                  << std::string(30 - QString((i.key()).toLocal8Bit().constData()).size(), ' ') << i.value().at(0)
                  << std::string(10 - QString(QString::number(i.value().at(0))).size(), ' ') << i.value().at(1)
                  << std::string(10 - QString(QString::number(i.value().at(1))).size(), ' ') << i.value().at(2)
                  << std::string(10 - QString(QString::number(i.value().at(2))).size(), ' ') << i.value().at(3)
                  << std::endl;
    }
    std::cout << std::string(70, '-') << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

    // Final Results
    std::cout << std::string(70, '-') << std::endl;
    std::cout << "Final Results" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    allLines = allCodeLines + allComments + allEmptyLines;
    std::cout << "All EmptyLines: "   << allEmptyLines << std::endl;
    std::cout << "All Comment Lines: " << allComments << std::endl;
    std::cout << "All Code Lines: " << allCodeLines << std::endl;
    std::cout << "All Files: " << allFiles << std::endl;
    std::cout << "All Lines: " << allLines << std::endl;
    std::cout << "Execution Time: "
              << QString::number(timer->elapsed() / 1000.00).toLocal8Bit().constData() << std::endl;

} // here ends the print function


void SourceCounter::count(const QString& nameOfFile, const QString &fileType)
{

    int numberFiles = 0;
    int emptyLines = 0;
    int commentLines = 0;
    int codeLines = 0;


    QFile file(nameOfFile);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        std::cerr << "The file cannot be open for reading!" << std::endl;


    QTextStream in(&file);
    while(!in.atEnd()) {
        QString line = in.readLine();

        while(line.endsWith(" "))
            line.chop(1);

        if(line.isEmpty())
            emptyLines++;

        else {
            QString commentL = line.trimmed();
            if( commentL.startsWith("//") )
                commentLines++;

            else
                codeLines++;
        }
    }

    file.close();

    numberFiles++;

    allCodeLines += codeLines;
    allComments += commentLines;
    allEmptyLines += emptyLines;
    allFiles += numberFiles;


    QVector<int> countVec;
    countVec.append(numberFiles);
    countVec.append(emptyLines);
    countVec.append(commentLines);
    countVec.append(codeLines);

    if( myMap->contains(fileType) ) {
        QVector<int> mapVec = myMap->value(fileType);
        Q_ASSERT(countVec.size() == mapVec.size());

        QVector<int> resultingVec;

        for(int i = 0; i != countVec.size(); ++i)
              resultingVec.append(countVec.at(i) + mapVec.at(i));

        myMap->insert(fileType, resultingVec);
    }

    else
        myMap->insert(fileType, countVec);
}

void SourceCounter::countCpp(const QString& nameOfFile, const QString &fileType)
{

    int numberFiles = 0;
    int emptyLines = 0;
    int commentLines = 0;
    int codeLines = 0;

    bool isInComment = false;

    QFile file(nameOfFile);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        std::cerr << "The file cannot be open for reading" << std::endl;

    QTextStream in(&file);
    while(!in.atEnd()) {
      QString line = in.readLine();

      while(line.endsWith(" ") || line.endsWith("\t"))
          line.chop(1);

      if(line.isEmpty())
          emptyLines++;

      else {
          QString commentL = line.trimmed();

          if( commentL.startsWith("//") )
              commentLines++;

          else if( commentL.startsWith("/*") ) {
              isInComment = true;

              if( commentL.contains("*/")) {

                  if(commentL.endsWith("*/")) {
                       commentLines++;
                       isInComment = false;
                  }

                  if(!commentL.endsWith("*/")) {
                       codeLines++;
                       isInComment = false;
                  }
              }
              else {
                  commentLines++;
                  isInComment = true;
              }

          }

          else if( isInComment ) {

              if( commentL.endsWith("*/")) {
                  commentLines++;
                  isInComment = false;
              }

              else {
                  commentLines++;
              }

          } // here ends isInComment test

          else
              codeLines++;

      }
  }

  file.close();

  numberFiles++;

  allCodeLines += codeLines;
  allComments += commentLines;
  allEmptyLines += emptyLines;
  allFiles += numberFiles;


  QVector<int> countVec;
  countVec.append(numberFiles);
  countVec.append(emptyLines);
  countVec.append(commentLines);
  countVec.append(codeLines);

  if( myMap->contains(fileType) ) {
      QVector<int> mapVec = myMap->value(fileType);
      Q_ASSERT(countVec.size() == mapVec.size());

      QVector<int> resultingVec;

      for(int i = 0; i != countVec.size(); ++i)
              resultingVec.append(countVec.at(i) + mapVec.at(i));

      myMap->insert(fileType, resultingVec);
  }

  else {
      myMap->insert(fileType, countVec);
  }

}
void SourceCounter::found(const QString& nameOfFolder)
{

     QDir dir(nameOfFolder);

     foreach (QFileInfo subDir, dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {

         if(subDir.isDir())
             found(subDir.filePath());

         if(subDir.isFile() && filtersPtr->contains(subDir.suffix())) {

              QString suffix = subDir.suffix();

              if(suffix == "cpp" || suffix == "cc" || suffix == "C" || suffix == "cxx" || suffix == "pcc")
                  countCpp(subDir.filePath(), "C++");

              else if(suffix == "H" || suffix == "h" || suffix == "hh" || suffix == "hpp")
                  countCpp(subDir.filePath(), "C/C++ Header");

              else if(suffix == "c" || suffix == "ec" || suffix == "pgc")
                  countCpp(subDir.filePath(), "C");

              else if(suffix == "py")
                  count(subDir.filePath(), "Python");

              else if(suffix == "XML" || suffix == "xml")
                  count(subDir.filePath(), "XML");

              else if(suffix == "html" || suffix == "htm")
                  count(subDir.filePath(), "HTML");

              else if(suffix == "java")
                  count(subDir.filePath(), "Java");

              else if(suffix == "js")
                  count(subDir.filePath(), "JavaScript");

              else if(suffix == "pyx")
                  count(subDir.filePath(), "Cython");

              else if(suffix == "el" || suffix == "jl" || suffix == "lisp"
                      || suffix == "lsp" || suffix == "sc" || suffix == "scm")
                  count(subDir.filePath(), "Lisp");

              else if(suffix == "perl" || suffix == "PL" || suffix == "pl"
                      || suffix == "plh" || suffix == "plx" || suffix == "pm")
                  count(subDir.filePath(), "Perl");

              else if(suffix == "php" || suffix == "php3" || suffix == "php4" || suffix == "php5")
                  count(subDir.filePath(), "PHP");

              else if(suffix == "rb")
                  count(subDir.filePath(), "Ruby");

              else if(suffix == "psql" || suffix == "SQL" || suffix == "sql")
                  count(subDir.filePath(), "SQL");

              else if(suffix == "csh" || suffix == "tcsh")
                  count(subDir.filePath(), "C Shell");

              else if(suffix == "abap")
                  count(subDir.filePath(), "ABAP");

              else if(suffix == "as")
                  count(subDir.filePath(), "ActionScript");

              else if(suffix == "ada" || suffix == "adb" || suffix == "ads" || suffix == "pad")
                  count(subDir.filePath(), "Ada");
              else if(suffix == "adso")
                  count(subDir.filePath(), "ADSO/IDSM");

              else if(suffix == "ample" || suffix == "dofile" || suffix == "startup")
                  count(subDir.filePath(), "AMPLE");

              else if(suffix == "asa" || suffix == "asp")
                  count(subDir.filePath(), "ASP");

              else if(suffix == "asax" || suffix == "ascx" || suffix == "asmx" ||
                      suffix == "aspx" || suffix == "config" || suffix == "master" ||
                      suffix == "sitemap" || suffix == "webinfo")
                  count(subDir.filePath(), "ASP.Net");

              else if(suffix == "asm" || suffix == "S" || suffix == "s")
                  count(subDir.filePath(), "Assembly");

              else if(suffix == "ahk")
                  count(subDir.filePath(), "AutoHotkey");

              else if(suffix == "awk")
                  count(subDir.filePath(), "awk");

              else if(suffix == "bash")
                  count(subDir.filePath(), "Bourne Again Shell");

              else if(suffix == "sh")
                  count(subDir.filePath(), "Bourne Shell");

              else if(suffix == "cs")
                  count(subDir.filePath(), "C#");

              else if(suffix == "ccs")
                  count(subDir.filePath(), "CCS");

              else if(suffix == "clj")
                  count(subDir.filePath(), "Clojure");

              else if(suffix == "cljs")
                  count(subDir.filePath(), "ClojureScript");

              else if(suffix == "CMakeLists.txt")
                  count(subDir.filePath(), "CMake");

              else if(suffix == "cbl" || suffix == "CBL" || suffix == "cob" || suffix == "COB")
                  count(subDir.filePath(), "COBOL");

              else if(suffix == "coffee")
                  count(subDir.filePath(), "CoffeeScript");

              else if(suffix == "cfm")
                  count(subDir.filePath(), "ColdFusion");

              else if(suffix == "cfc")
                  count(subDir.filePath(), "ColdFusion CFScript");

              else if(suffix == "css")
                  count(subDir.filePath(), "CSS");

              else if(suffix == "d")
                  count(subDir.filePath(), "D");

              else if(suffix == "da")
                  count(subDir.filePath(), "DAL");

              else if(suffix == "dart")
                  count(subDir.filePath(), "Dart");

              else if(suffix == "bat" || suffix == "BAT")
                  count(subDir.filePath(), "DOS Batch");

              else if(suffix == "dtd")
                  count(subDir.filePath(), "DTD");

              else if(suffix == "erl" || suffix == "hrl")
                  count(subDir.filePath(), "Erlang");

              else if(suffix == "exp")
                  count(subDir.filePath(), "Expect");

              else if(suffix == "focexec")
                  count(subDir.filePath(), "Focus");

              else if(suffix == "F" || suffix == "f" || suffix == "f77" || suffix == "F77" || suffix == "pfo")
                  count(subDir.filePath(), "Fortran 77");

              else if(suffix == "F90" || suffix == "f90")
                  count(subDir.filePath(), "Fortran 90");

              else if(suffix == "F95" || suffix == "f95")
                  count(subDir.filePath(), "Fortran 95");

              else if(suffix == "go")
                  count(subDir.filePath(), "Go");

              else if(suffix == "groovy")
                  count(subDir.filePath(), "Groovy");

              else if(suffix == "hs" || suffix == "lhs")
                  count(subDir.filePath(), "Haskell");

              else if(suffix == "idl" /*|| suffix == "pro" */)
                  count(subDir.filePath(), "IDL");

              else if(suffix == "jcl")
                  count(subDir.filePath(), "JCL");

              else if(suffix == "jsp")
                  count(subDir.filePath(), "JSP");

              else if(suffix == "ksc")
                  count(subDir.filePath(), "Kermit");

              else if(suffix == "ksh")
                  count(subDir.filePath(), "Korn Shell");

              else if(suffix == "l")
                  count(subDir.filePath(), "lex");

              else if(suffix == "cl")
                  count(subDir.filePath(), "Lisp/OpenCL");

              else if(suffix == "oscript")
                  count(subDir.filePath(), "LiveLink OScript");

              else if(suffix == "lua")
                  count(subDir.filePath(), "Lua");

              else if(suffix == "ac" || suffix == "m4")
                  count(subDir.filePath(), "m4");

              else if(suffix == "am" || suffix == "gnumakefile" || suffix == "Gnumakefile"
                      || suffix == "Makefile" || suffix == "makefile")
                  count(subDir.filePath(), "make");

//              else if(suffix == "m")
//                  count(subDir.filePath(), "MATLAB");

              else if(suffix == "i3" || suffix == "ig" || suffix == "m3" || suffix == "mg")
                  count(subDir.filePath(), "Modula3");

              else if(suffix == "csproj" || suffix == "wdproj")
                  count(subDir.filePath(), "MSBuild scripts");

              else if(suffix == "mps" /*suffix == "m" */)
                  count(subDir.filePath(), "MUMPS");

              else if(suffix == "mxml")
                  count(subDir.filePath(), "MXML");

              else if(suffix == "build")
                  count(subDir.filePath(), "NAnt scripts");

              else if(suffix == "dmap")
                  count(subDir.filePath(), "NASTRAN DMAP");

              else if(suffix == "m")
                  count(subDir.filePath(), "Objective C");

              else if(suffix == "mm")
                  count(subDir.filePath(), "Objective C++");

              else if(suffix == "ml")
                  count(subDir.filePath(), "Ocaml");

              else if(suffix == "fmt")
                  count(subDir.filePath(), "Oracle Forms");

              else if(suffix == "rex")
                  count(subDir.filePath(), "Oracle Reports");

              else if(suffix == "dpr" || suffix == "p" || suffix == "pas" || suffix == "pp")
                  count(subDir.filePath(), "Pascal");

              else if(suffix == "pcl" || suffix == "ses")
                  count(subDir.filePath(), "Patran Command Language");

              else if(suffix == "inc")
                  count(subDir.filePath(), "PHP/Pascal");

              else if(suffix == "qml")
                  count(subDir.filePath(), "QML");

              else if(suffix == "rexx")
                  count(subDir.filePath(), "Rexx");

              else if(suffix == "rhtml")
                  count(subDir.filePath(), "Ruby HTML");

              else if(suffix == "scala")
                  count(subDir.filePath(), "Scala");

              else if(suffix == "sed")
                  count(subDir.filePath(), "sed");

              else if(suffix == "il")
                  count(subDir.filePath(), "SKILL");

              else if(suffix == "ils")
                  count(subDir.filePath(), "SKILL++");

              else if(suffix == "smarty" || suffix == "tpl")
                  count(subDir.filePath(), "Smarty");

              else if(suffix == "sbl" || suffix == "SBL")
                  count(subDir.filePath(), "Softbridge Basic");

              else if(suffix == "data.sql")
                  count(subDir.filePath(), "SQL Data");

              else if(suffix == "spc.sql" || suffix == "spoc.sql" || suffix == "sproc.sql" || suffix == "udf.sql")
                  count(subDir.filePath(), "SQL Stored Procedure");

              else if(suffix == "itk" || suffix == "tcl" || suffix == "tk")
                  count(subDir.filePath(), "Tcl/Tk");

              else if(suffix == "def")
                  count(subDir.filePath(), "Teamcenter def");

              else if(suffix == "met")
                  count(subDir.filePath(), "Teamcenter met");

              else if(suffix == "mth")
                  count(subDir.filePath(), "Teamcenter mth");

              else if(suffix == "vhd" || suffix == "VHD" || suffix == "VHDL" || suffix == "vhdl")
                  count(subDir.filePath(), "VHDL");

              else if(suffix == "vim")
                  count(subDir.filePath(), "vim script");

              else if(suffix == "bas" || suffix == "cls" || suffix == "ctl" || suffix == "dsr"
                      || suffix == "frm" || suffix == "vb" || suffix == "VB" || suffix == "vba"
                      || suffix == "VBA" || suffix == "vbs" || suffix == "VBS")
                  count(subDir.filePath(), "Visual Basic");

              else if(suffix == "xaml")
                  count(subDir.filePath(), "XAML");

              else if(suffix == "xsd" || suffix == "XSD")
                  count(subDir.filePath(), "XSD");

              else if(suffix == "xsl" || suffix == "XSL" || suffix == "xslt" || suffix == "XSLT")
                  count(subDir.filePath(), "XSLT");

              else if(suffix == "y")
                  count(subDir.filePath(), "yacc");

              else if(suffix == "yaml" || suffix == "yml")
                  count(subDir.filePath(), "YAML");

        } // here ends the file checking

     } // here ends the foreach loop

} // here ends the function


void SourceCounter::initVariables()
{

    myMap = new QMap<QString, QVector<int> >;

    allLines = 0;
    allCodeLines = 0;
    allEmptyLines = 0;
    allComments = 0;
    allFiles = 0;

    filtersPtr = new QStringList;
    QStringList filters;

    filters << "cpp" << "h" << "c" << "py" << "pyx" << "C" << "H" << "cc" << "cxx" << "pcc" << "html" << "htm"
            << "hh" << "hpp" << "ec" << "pgc" << "bash" << "sh" << "java" << "js"
            << "el" << "jl" << "lisp" << "lsp" << "sc" << "scm" << "perl" << "rb"
            << "PL" << "pl" << "plh" << "plx" << "pm" << "php" << "php3" << "php4" << "php5"
            << "psql" << "SQL" << "sql"
            << "abap" << "as" << "ada" << "adb" << "ads" << "pad"
            << "adso" << "ample" << "dofile" << "startup" << "asa" << "asp"
            << "asax" << "ascx" << "asmx" << "aspx" << "config" << "master" << "sitemap" << "webinfo"
            << "asm" << "S" << "s" << "ahk" << "awk" << "csh" << "tcsh"
            << "cs" << "ccs" << "clj" << "cljs" << "CMakeLists.txt"
            << "cbl" << "CBL" << "cob" << "COB" << "coffee" << "cfm" << "cfc" << "css"
            << "d" << "da" << "dart" << "bat" << "BAT" << "dtd" << "erl" << "hrl" << "exp"
            << "focexec" << "F" << "f" << "f77" << "F77" << "pfo" << "F90" << "f90" << "F95" << "f95"
            << "go" << "groovy" << "hs" << "lhs" << "idl" << "pro" << "jcl" << "jsp" << "ksc"
            << "ksh" << "l" << "cl" << "oscript" << "lua"
            << "ac" << "m4" << "am" << "gnumakefile" << "Gnumakefile" << "Makefile" << "makefile" << "m"
            << "i3" << "ig" << "m3" << "mg" << "csproj" << "wdproj" << "mps" << "mxml" << "build" << "dmap"
            << "mm" << "ml" << "fmt" << "rex" << "dpl" << "p" << "pas" << "pp" << "pcl" << "ses" << "inc"
            << "qml" << "rexx" << "rhtml" << "scala" << "sed" << "il" << "ils" << "smarty" << "tpl"
            << "sbl" << "SBL" << "data.sql" << "def" << "spc.sql" << "spoc.sql"
            << "sproc.sql" << "udf.sql" << "itk" << "tcl" << "tk" << "met" << "mth" << "vhd" << "VHD" << "VHDL"
            << "vhdl" << "vim" << "bas" << "cls" << "ctl" << "dsr" << "frm" << "vb" << "VB" << "vba" << "VBA"
            << "vbs" << "VBS";

    filtersPtr->append(filters);
}
