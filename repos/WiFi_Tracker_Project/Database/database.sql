create table if not exists rooms (roomname varchar, xlength real, ylength real, pcount int);
create table if not exists stations (namemac varchar, roomname varchar, xpos real, ypos real, shortintrp varchar, longintrp varchar);
create table if not exists devicespositions (identifier varchar, mac varchar, roomname varchar, xpos real, ypos real,uncertainty real, tm timestamp, outmovement bool,xhour int, xday int,xmonth int,xyear int);
create table if not exists requestedssids (identifier varchar, ssid varchar);
create table if not exists countstats (roomname varchar, count real, tm timestamp,xhour int, xday int,xmonth int,xyear int);

alter table countstats add column cat int;
alter table rooms add column archived boolean;
alter table requestedssids add constraint pc unique (identifier, ssid);
alter table rooms add constraint rnuq unique(roomname);
alter table stations add constraint stuq unique (namemac);
GRANT CONNECT, TEMPORARY ON DATABASE panopticon TO panopticon;
GRANT ALL ON DATABASE panopticon TO panopticon;
grant all on rooms,devicespositions,requestedssids,countstats,stations to panopticon;