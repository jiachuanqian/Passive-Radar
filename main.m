clc
close all
clear
k = 1000;
sr = 10e6;
c = 300e6;
% p = 'C:\Users\Administrator\Desktop\new\build\Release\outdata\0\';
p = 'build/outdata/0/';
ff = dir([p,'*data*']);
fileNames = {ff.name};

nums = zeros(length(fileNames),1);
for i = 1:length(fileNames)
    tokens = regexp(fileNames{i}, 'out_([-+]?\d+)\.data', 'tokens');
    if ~isempty(tokens)
        nums(i) = str2double(tokens{1}{1});
    else
        nums(i) = NaN;
    end
end
[~, idx] = sort(nums);
sortedFileNames = fileNames(idx);

for i = 1:length(fileNames)
    fid = fopen([p,sortedFileNames{i}],'r');
    RC = fread(fid,'single');
    RC = (RC(1:2:end)+1j*RC(2:2:end));
    RCC = reshape(RC,k,[]);
    temp = abs(RCC).^2;
    noise =  db(mean(mean(temp(1:1000,1:190))))/2;
    
    %% 
    figure,mesh(((-size(RCC,2)+1)/2:(size(RCC,2)-1)/2),(1:k-1)*c/sr,db(RCC(2:end,:)))
    % hold on, mesh(((-size(RCC,2)+1)/2:(size(RCC,2)-1)/2),(0:k-1)*c/sr,ones(size(RCC)) * noise ,'EdgeColor', 'r')
    xlabel("频率/Hz"); ylabel("距离/m"); zlabel("dB")
    title("测向角度："+ num2str(i-round(length(fileNames)/2)) + "°" +"噪声平均功率：" + sprintf('%.2f', noise) +"dB")
    view(0,0)
    fclose(fid);
end



