-- 174: which node owns an instance's process.
--
-- ga_instances.node_id holds the node id that owns the instance (0 = the master
-- itself). The multi-node work used to add it from the control server's startup
-- ladder, unconditionally, on every boot. With the ladder gone this is the only
-- thing that introduces it -- and it has to, because a database downloaded from
-- a server that predates the multi-node work reports version 173 and has no such
-- column, so the version number alone does not mean "this schema".

ALTER TABLE ga_instances ADD COLUMN node_id INTEGER NOT NULL DEFAULT 0;

-- GetInstancesForNode and CountActiveInstancesForNode filter on this. ga_instances
-- is ~13k rows on a server that has been up a while, so it needs the index.
CREATE INDEX IF NOT EXISTS idx_ga_instances_node ON ga_instances(node_id);
