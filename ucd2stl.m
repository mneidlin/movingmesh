%%%%%%% CODE WRITTE BY JANA KORTE IN 2020%%%%
%Contact: Michael Neidlin, neidlin@ame.rwth-aachen.de%%

%Background can be found at
% https://www5.in.tum.de/lehre/praktika/grapra/avsdoku/bauinfo/Bericht/html/kapitel5_main.html

%% Read UCD-File and extract data
% REMARK: Adjust file and folder names whenever necessary


numLines = 3752;
xx = 0;

for xx = 0:16
    if xx < 10
        filename1 = sprintf('5248_200928_JKO_0%d.ucd',xx);
    else
        filename1 = sprintf('5248_200928_JKO_%d.ucd',xx);
    end

fileID = fopen(filename1, 'r');

% read data
mydata = cell(1, numLines);
for k = 1:numLines
   mydata{k} = fgetl(fileID); %von JSE
%    mydata{k} = fopen(fileID);
end
fclose(fileID);

% headerline: first line of ucd file
headerline = str2num(mydata{1});
nn = headerline(1);   % number of nodes
ne = headerline(2);   % number of elements
ncnd = headerline(3); % number of columns of node data
nccd = headerline(4); % number of columns of cell data

% node coordinates
% format: id x y z
for i = 2 : nn+1
    nodecoords(i-1,:) = str2num(mydata{i});
end

% element structure
% format: n1 n2 n3 (no ID!!!)
for i = nn+2 : nn+ne+1
    tmp = split(mydata{i});
    tmp2 = string(tmp(4:6));
    % TODO verify that tmp(3) = 'tri'
    for j = 1:3
        elemstructure(i-(nn+1),j) = double(tmp2(j));
    end
end



%% create STL-File

sizeSTL = ne*7 + 2;
contentSTL = cell(1,sizeSTL);

% bring ucd data into stl structure 
contentSTL{1} = 'solid ascii';
for i = 1:ne
    contentSTL{(i-1)*7+2} = 'facet normal 0 0 0';
    contentSTL{(i-1)*7+3} = 'outer loop';
    contentSTL{(i-1)*7+4} = join(['vertex ' string(nodecoords(elemstructure(i,1)+1,2)) ' ' string(nodecoords(elemstructure(i,1)+1,3)) ' ' string(nodecoords(elemstructure(i,1)+1,4))], '');
    contentSTL{(i-1)*7+5} = join(['vertex ' string(nodecoords(elemstructure(i,2)+1,2)) ' ' string(nodecoords(elemstructure(i,2)+1,3)) ' ' string(nodecoords(elemstructure(i,2)+1,4))], '');
    contentSTL{(i-1)*7+6} = join(['vertex ' string(nodecoords(elemstructure(i,3)+1,2)) ' ' string(nodecoords(elemstructure(i,3)+1,3)) ' ' string(nodecoords(elemstructure(i,3)+1,4))], '');
    contentSTL{(i-1)*7+7} = 'endloop';
    contentSTL{(i-1)*7+8} = 'endfacet';
end
contentSTL{sizeSTL} = 'endsolid';

% write STL file
filename = sprintf('ventricle_%d.stl',xx);
folder = ['\\CVe-SIm\sim_studenten\Jana_Korte_JKO\WIHI_SH\02_Technische_Entwicklung\01_Konstruktion\00_UCD-Datenumwandlung-STL\5248_200928_JKO_\' filename];
fileID = fopen(folder,'w');
fprintf(fileID,'%s\n',contentSTL{:});
fclose(fileID);
end