%% Created by Chenshuo 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%Plotting of Raw data and Processing
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%
%LINK RATE PLOT
%%%%%%%%%%%%%%%%%%%%%
clc
clear
subplot(6,1,1)

sp_rate = 100;
number = 5;% number link rate file from LinkRate0.txt to LinkRate'number'.txt
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
k = number -1; % 
FileName_array = ['LinkRate' num2str(0) '.txt']; % 
legendName_array = ['L' num2str(1)];
for i = 1:k
    textFileName= ['LinkRate' num2str(i) '.txt'];% 
    legendName = ['L' num2str(i+1)];
    FileName_array = char(FileName_array, textFileName);
    legendName_array = char(legendName_array, legendName);
end
FileName_array = cellstr(FileName_array);

for j = 1:k+1
    file_name = char(FileName_array(j));
    fileID = fopen(file_name,'r');
    formatSpec = '%f %f';
    sizeA = [2 Inf];
    A = fscanf(fileID,formatSpec,sizeA);
    x = A(1,:);
    y = A(2,:);
    LinkRate(j).time = x;
    LinkRate(j).pkgsize = y;
    fclose(fileID);
end
for j =2:3 % refer to L1, L2, L3
    time_out = ceil(LinkRate(j).time(end));
    % time_out = ceil(LinkRate(j).time(end));
    t_plot = linspace(0,0,length(0:sp_rate:time_out));
    flow_rate = linspace(0,0,length(0:sp_rate:time_out));
    i = 1;
    for t = 0:sp_rate:time_out
        idx = find(LinkRate(j).time < t & LinkRate(j).time > t - sp_rate);
        total_pkg = sum(LinkRate(j).pkgsize(idx));
        t_plot(i) = t/1000;
        flow_rate(i) = (total_pkg / sp_rate)/1000;
        i = i + 1;
    end
    plot(t_plot,flow_rate)
    hold on;
end
ylabel('link rate (Mbps)')
xlabel('time(s)')
legend('L1','L2','L3')
%
%%%%%%%%%%%%%%%%%%%%%
%BUFFER OCCOPANCY PLOT
%%%%%%%%%%%%%%%%%%%%%
subplot(6,1,2)
sp_rate = 200;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
number = 5;% number link buffer file from LinkBuffer0.txt to LinkBuffer'number'.txt
k = number -1; % 
FileName_array = ['LinkBuffer' num2str(0) '.txt']; % 
legendName_array = ['L' num2str(1)];
for i = 1:k
    textFileName= ['LinkBuffer' num2str(i) '.txt'];% 
    legendName = ['L' num2str(i+1)];
    FileName_array = char(FileName_array, textFileName);
    legendName_array = char(legendName_array, legendName);
end
FileName_array = cellstr(FileName_array);

for j = 1:k+1
    file_name = char(FileName_array(j));
    fileID = fopen(file_name);
    formatSpec = '%f %f %f';
    sizeA = [3 Inf];
    A = fscanf(fileID,formatSpec,sizeA);
    x = A(1,:);
    y = A(2,:);
    LinkBuffer(j).time = x/1000;
    LinkBuffer(j).pkgsize = y;
    fclose(fileID);
end

for j = 2:3% this is target link will be plotted,please be consistant with legend
    plot(LinkBuffer(j).time,LinkBuffer(j).pkgsize)
    hold on;
end
ylabel('buffer occupancy(pkts)')
xlabel('time(s)')
legend('L1','L2','L3')% remember to change this to be corresponding with the target lin
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%%%%%%%%%%%%%%%%%%%%%
%PACKET LOSS PLOT
%%%%%%%%%%%%%%%%%%%%%
subplot(6,1,3)

number = 3;% number PacketLoss file from PackketLoss0.txt to PacketLoss'number'.txt
k = number -1; % 
FileName_array = ['PacketLoss' num2str(0) '.txt']; % 
legendName_array = ['L' num2str(1)];
for i = 1:k
    textFileName= ['PacketLoss' num2str(i) '.txt'];% 
    legendName = ['L' num2str(i+1)];
    FileName_array = char(FileName_array, textFileName);
    legendName_array = char(legendName_array, legendName);
end
FileName_array = cellstr(FileName_array);
%
for j = 1:number
    file_name = char(FileName_array(j));
    fileID = fopen(file_name,'r');
    if fileID > 0
        formatSpec = '%f %f';
        sizeA = [2 Inf];
        A = fscanf(fileID,formatSpec,sizeA);
        x = A(1,:);
        y = A(2,:);
        PacketLoss(j).time = x;
        PacketLoss(j).pkgsize = y;
        fclose(fileID);
    else
        PacketLoss(j).time = LinkRate(1).time;
        PacketLoss(j).pkgsize = linspace(0,0,length(PacketLoss(j).time));
    end
end
%
%
interval = 30;
for j = 2:3 %refer to L1, L2, L3
    time_out = ceil(LinkRate(1).time(end));
    t_plot = linspace(0,0,length(0:interval:time_out));
    pkts_loss = linspace(0,0,length(0:interval:time_out));
    i = 1;
    for t = 0:interval:time_out
%         plot(PacketLoss(j).time,PacketLoss(j).pkgsize,':')
%         hold on
        idx = find(PacketLoss(j).time < t & PacketLoss(j).time > t - interval);
        total_pkg = sum(PacketLoss(j).pkgsize(idx));
        t_plot(i) = t/1000;
        pkts_loss(i) = total_pkg/interval;
        i = i + 1;
    end
    plot(t_plot,pkts_loss)
    hold on;
end
axis([0 inf 0 inf])
ylabel('packet loss(pkts)')
xlabel('time(s)')
legend(legendName_array)% remember to change this to be the target Link
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%%%%%%%%%%%%%%%%%%%%%
%FLOW RATE PLOT
%%%%%%%%%%%%%%%%%%%%%
subplot(6,1,4)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
sp_rate = 100;
number = 1;% number Flow rate file from FlowRate0.txt to FlowRate'number'.txt
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
k = number -1; % 
FileName_array = ['FlowRate' num2str(0) '.txt']; % 
legendName_array = ['flow' num2str(1)];
for i = 1:k
    textFileName= ['FlowRate' num2str(i) '.txt'];% 
    legendName = ['flow' num2str(i+1)];
    FileName_array = char(FileName_array, textFileName);
    legendName_array = char(legendName_array, legendName);
end
FileName_array = cellstr(FileName_array);

for j = 1:k+1
    file_name = char(FileName_array(j));
    fileID = fopen(file_name,'r');
    formatSpec = '%f %f %f';
    sizeA = [3 Inf];
    A = fscanf(fileID,formatSpec,sizeA);
    x = A(1,:);
    y = A(2,:);
    rtt = A(3,:);
    FlowRate(j).time = x;
    FlowRate(j).pkgsize = y;
    fclose(fileID);
end
%
for j =1:k+1
    time_out = ceil(FlowRate(j).time(end));
    % time_out = ceil(LinkRate(j).time(end));
    t_plot = linspace(0,0,length(0:sp_rate:time_out));
    flow_rate = linspace(0,0,length(0:sp_rate:time_out));
    i = 1;
    for t = 0:sp_rate:time_out
        idx = find(FlowRate(j).time < t & FlowRate(j).time > t - sp_rate);
        total_pkg = sum(FlowRate(j).pkgsize(idx));
        t_plot(i) = t/1000;
        flow_rate(i) = (total_pkg / sp_rate)/1000;
        i = i + 1;
    end
    plot(t_plot,flow_rate)
    hold on;
end
ylabel('flow rate (Mbps)')
xlabel('time(s)')
legend(legendName_array)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%%%%%%%%%%%%%%%%%%%%%
%WINDOW SIZE PLOT
%%%%%%%%%%%%%%%%%%%%%
subplot(6,1,5)
sp_rate = 150;
number = 1;% number window size file
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
k = number -1; % 
FileName_array = ['WindSize' num2str(0) '.txt']; % 
legendName_array = ['flow' num2str(1)];
for i = 1:k
    textFileName= ['WindSize' num2str(i) '.txt'];% 
    legendName = ['flow' num2str(i+1)];
    FileName_array = char(FileName_array, textFileName);
    legendName_array = char(legendName_array, legendName);
end
FileName_array = cellstr(FileName_array);
max_wid = linspace(0,0,number);
for j = 1:k+1
    file_name = char(FileName_array(j));
    fileID = fopen(file_name,'r');
    formatSpec = '%f %f';
    sizeA = [2 Inf];
    A = fscanf(fileID,formatSpec,sizeA);
    x = A(1,:);
    y = A(2,:);
    WindSize(j).time = x/1000;
    WindSize(j).pkgsize = y;
    max_wid(j) = max(WindSize(j).pkgsize);
    fclose(fileID);
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% change 'sample.txt' to the name of destination file 
for j = 1:number
    plot(WindSize(j).time,WindSize(j).pkgsize)
    hold on;
end
max_yaxis = max(max_wid);
axis([0 inf 0 max_yaxis])
ylabel('window size(pkts)')
xlabel('time(s)')
legend(legendName_array)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%%%%%%%%%%%%%%%%%%%%%
%PACKET DELAY PLOT
%%%%%%%%%%%%%%%%%%%%%
subplot(6,1,6)
sp_rate = 100;
number = 1;% number PacketDelay file
k = number -1; % 
FileName_array = ['PacketDelay' num2str(0) '.txt']; % 
legendName_array = ['flow' num2str(1)];
for i = 1:k
    textFileName= ['PacketDelay' num2str(i) '.txt'];% 
    legendName = ['flow' num2str(i+1)];
    FileName_array = char(FileName_array, textFileName);
    legendName_array = char(legendName_array, legendName);
end
FileName_array = cellstr(FileName_array);
max_delay = linspace(0,0,number);
for j = 1:k+1
    file_name = char(FileName_array(j));
    fileID = fopen(file_name,'r');
    formatSpec = '%f %f';
    sizeA = [2 Inf];
    A = fscanf(fileID,formatSpec,sizeA);
    x = A(1,:);
    y = A(2,:);
    PacketDelay(j).time = x/1000;
    PacketDelay(j).pkgsize = y;
    max_delay(j) = max(PacketDelay(j).pkgsize);
    fclose(fileID);
end
%
time_out = ceil(PacketDelay(1).time(end));
%

for j = 1:number
    plot(PacketDelay(j).time,PacketDelay(j).pkgsize)
    hold on;
end
max_yaxis = max(max_delay);
axis([0 inf 0 max_yaxis])
ylabel('packet delay(rms)')
xlabel('time(s)')
legend(legendName_array)
%
