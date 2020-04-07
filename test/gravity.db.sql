PRAGMA FOREIGN_KEYS=ON;
BEGIN TRANSACTION;

CREATE TABLE "group"
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	enabled BOOLEAN NOT NULL DEFAULT 1,
	name TEXT NOT NULL,
	date_added INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int)),
	date_modified INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int)),
	description TEXT
);

INSERT OR REPLACE INTO "group" (id,enabled,name) VALUES (0,1,'Unassociated');
CREATE TRIGGER tr_group_zero AFTER DELETE ON "group"
    BEGIN
      INSERT OR REPLACE INTO "group" (id,enabled,name) VALUES (0,1,'Unassociated');
    END;
CREATE TRIGGER tr_group_update AFTER UPDATE ON "group"
    BEGIN
      UPDATE "group" SET date_modified = (cast(strftime('%s', 'now') as int)) WHERE id = NEW.id;
    END;

CREATE TABLE domainlist
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	type INTEGER NOT NULL DEFAULT 0,
	domain TEXT UNIQUE NOT NULL,
	enabled BOOLEAN NOT NULL DEFAULT 1,
	date_added INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int)),
	date_modified INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int)),
	comment TEXT
);
CREATE TRIGGER tr_domainlist_update AFTER UPDATE ON domainlist
    BEGIN
      UPDATE domainlist SET date_modified = (cast(strftime('%s', 'now') as int)) WHERE domain = NEW.domain;
    END;
CREATE TRIGGER tr_domainlist_add AFTER INSERT ON domainlist
    BEGIN
      INSERT INTO domainlist_by_group (domainlist_id, group_id) VALUES (NEW.id, 0);
    END;

CREATE TABLE domainlist_by_group
(
        domainlist_id INTEGER NOT NULL REFERENCES domainlist (id),
        group_id INTEGER NOT NULL REFERENCES "group" (id),
        PRIMARY KEY (domainlist_id, group_id)
);

CREATE TABLE adlist
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	address TEXT UNIQUE NOT NULL,
	enabled BOOLEAN NOT NULL DEFAULT 1,
	date_added INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int)),
	date_modified INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int)),
	comment TEXT
);
CREATE TRIGGER tr_adlist_update AFTER UPDATE ON adlist
    BEGIN
      UPDATE adlist SET date_modified = (cast(strftime('%s', 'now') as int)) WHERE address = NEW.address;
    END;
CREATE TRIGGER tr_adlist_add AFTER INSERT ON adlist
    BEGIN
      INSERT INTO adlist_by_group (adlist_id, group_id) VALUES (NEW.id, 0);
    END;

CREATE TABLE adlist_by_group
(
	adlist_id INTEGER NOT NULL REFERENCES adlist (id),
	group_id INTEGER NOT NULL REFERENCES "group" (id),
	PRIMARY KEY (adlist_id, group_id)
);

CREATE TABLE gravity
(
	domain TEXT NOT NULL,
	adlist_id INTEGER NOT NULL REFERENCES adlist (id),
	PRIMARY KEY(domain, adlist_id)
);

CREATE TABLE client
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	ip TEXT NOL NULL UNIQUE
);

CREATE TABLE client_by_group
(
	client_id INTEGER NOT NULL REFERENCES client (id),
	group_id INTEGER NOT NULL REFERENCES "group" (id),
	PRIMARY KEY (client_id, group_id)
);

CREATE TRIGGER tr_client_add AFTER INSERT ON client
    BEGIN
      INSERT INTO client_by_group (client_id, group_id) VALUES (NEW.id, 0);
    END;

CREATE TABLE domain_audit
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	domain TEXT UNIQUE NOT NULL,
	date_added INTEGER NOT NULL DEFAULT (cast(strftime('%s', 'now') as int))
);

CREATE TABLE info
(
	property TEXT PRIMARY KEY,
	value TEXT NOT NULL
);

CREATE VIEW vw_gravity AS SELECT domain, adlist_by_group.group_id AS group_id
    FROM gravity
    LEFT JOIN adlist_by_group ON adlist_by_group.adlist_id = gravity.adlist_id
    LEFT JOIN adlist ON adlist.id = gravity.adlist_id
    LEFT JOIN "group" ON "group".id = adlist_by_group.group_id
    WHERE adlist.enabled = 1 AND (adlist_by_group.group_id IS NULL OR "group".enabled = 1);

CREATE VIEW vw_whitelist AS SELECT domain, domainlist.id AS id, domainlist_by_group.group_id AS group_id
    FROM domainlist
    LEFT JOIN domainlist_by_group ON domainlist_by_group.domainlist_id = domainlist.id
    LEFT JOIN "group" ON "group".id = domainlist_by_group.group_id
    WHERE domainlist.enabled = 1 AND (domainlist_by_group.group_id IS NULL OR "group".enabled = 1)
    AND domainlist.type = 0
    ORDER BY domainlist.id;

CREATE VIEW vw_blacklist AS SELECT domain, domainlist.id AS id, domainlist_by_group.group_id AS group_id
    FROM domainlist
    LEFT JOIN domainlist_by_group ON domainlist_by_group.domainlist_id = domainlist.id
    LEFT JOIN "group" ON "group".id = domainlist_by_group.group_id
    WHERE domainlist.enabled = 1 AND (domainlist_by_group.group_id IS NULL OR "group".enabled = 1)
    AND domainlist.type = 1
    ORDER BY domainlist.id;

CREATE VIEW vw_regex_whitelist AS SELECT domain, domainlist.id AS id, domainlist_by_group.group_id AS group_id
    FROM domainlist
    LEFT JOIN domainlist_by_group ON domainlist_by_group.domainlist_id = domainlist.id
    LEFT JOIN "group" ON "group".id = domainlist_by_group.group_id
    WHERE domainlist.enabled = 1 AND (domainlist_by_group.group_id IS NULL OR "group".enabled = 1)
    AND domainlist.type = 2
    ORDER BY domainlist.id;

CREATE VIEW vw_regex_blacklist AS SELECT domain, domainlist.id AS id, domainlist_by_group.group_id AS group_id
    FROM domainlist
    LEFT JOIN domainlist_by_group ON domainlist_by_group.domainlist_id = domainlist.id
    LEFT JOIN "group" ON "group".id = domainlist_by_group.group_id
    WHERE domainlist.enabled = 1 AND (domainlist_by_group.group_id IS NULL OR "group".enabled = 1)
    AND domainlist.type = 3
    ORDER BY domainlist.id;

CREATE VIEW vw_adlist AS SELECT DISTINCT address, adlist.id AS id
    FROM adlist
    LEFT JOIN adlist_by_group ON adlist_by_group.adlist_id = adlist.id
    LEFT JOIN "group" ON "group".id = adlist_by_group.group_id
    WHERE adlist.enabled = 1 AND (adlist_by_group.group_id IS NULL OR "group".enabled = 1)
    ORDER BY adlist.id;

INSERT INTO domainlist VALUES(1,0,'whitelisted.test.pi-hole.net',1,1559928803,1559928803,'Migrated from /etc/pihole/whitelist.txt');
INSERT INTO domainlist VALUES(2,0,'regex1.test.pi-hole.net',1,1559928803,1559928803,'');
INSERT INTO domainlist VALUES(3,2,'regex2',1,1559928803,1559928803,'');
INSERT INTO domainlist VALUES(4,2,'discourse',1,1559928803,1559928803,'');

INSERT INTO domainlist VALUES(5,1,'blacklist-blocked.test.pi-hole.net',1,1559928803,1559928803,'Migrated from /etc/pihole/blacklist.txt');
INSERT INTO domainlist VALUES(6,3,'regex[0-9].test.pi-hole.net',1,1559928803,1559928803,'Migrated from /etc/pihole/regex.list');

INSERT INTO adlist VALUES(1,'https://hosts-file.net/ad_servers.txt',1,1559928803,1559928803,'Migrated from /etc/pihole/adlists.list');

INSERT INTO gravity VALUES('whitelisted.test.pi-hole.net',1);
INSERT INTO gravity VALUES('gravity-blocked.test.pi-hole.net',1);
INSERT INTO gravity VALUES('discourse.pi-hole.net',1);
INSERT INTO info VALUES("gravity_count",3);

INSERT INTO "group" VALUES(1,0,'Test group',1559928803,1559928803,'A disabled test group');
INSERT INTO domainlist VALUES(7,1,'blacklisted-group-disabled.com',1,1559928803,1559928803,'Entry disabled by a group');
INSERT INTO domainlist_by_group VALUES(7,1);

INSERT INTO domain_audit VALUES(1,'google.com',1559928803);

INSERT INTO client VALUES(1,"127.0.0.1");

INSERT INTO client VALUES(2,"127.0.0.2");
INSERT INTO "group" VALUES(2,1,"Second test group",1559928803,1559928803,"A group associated with client 127.0.0.2");
DELETE FROM client_by_group WHERE client_id = 2 AND group_id = 0;
INSERT INTO client_by_group VALUES(2,2);
INSERT INTO adlist_by_group VALUES(1,2);
INSERT INTO domainlist_by_group VALUES(6,2);

INSERT INTO client VALUES(3,"127.0.0.3");
INSERT INTO "group" VALUES(3,1,"Third test group",1559928803,1559928803,"A group associated with client 127.0.0.3");
DELETE FROM client_by_group WHERE client_id = 3 AND group_id = 0;
INSERT INTO client_by_group VALUES(3,3);

INSERT INTO info VALUES("version","9");

COMMIT;
