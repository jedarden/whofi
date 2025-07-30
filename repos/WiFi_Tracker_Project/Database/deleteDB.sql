ALTER TABLE requestedssids
DROP CONSTRAINT pc;

ALTER TABLE rooms
DROP CONSTRAINT rnuq;

ALTER TABLE stations
DROP CONSTRAINT stuq;

drop table rooms;
drop table stations;
drop table devicespositions;
drop table requestedssids;
drop table countstats;
