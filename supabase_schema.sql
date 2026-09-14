-- FactoMan: Supabase Database Schema for Multi-User Server Sync
-- Run this SQL in your Supabase project's SQL Editor (https://supabase.com/dashboard)

-- 1. Create the server_state table
CREATE TABLE IF NOT EXISTS public.server_state (
    id INT PRIMARY KEY DEFAULT 1,
    status TEXT NOT NULL DEFAULT 'OFFLINE', -- OFFLINE, STARTING, RUNNING, STOPPING
    server_ip TEXT NOT NULL DEFAULT '',
    launch_id BIGINT NOT NULL DEFAULT 0,
    region TEXT NOT NULL DEFAULT 'ap-south-1',
    save_slot TEXT NOT NULL DEFAULT 'slot1',
    version TEXT NOT NULL DEFAULT '2.1.17',
    updated_by TEXT NOT NULL DEFAULT 'System',
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- 2. Insert the initial singleton row (id = 1) if it doesn't already exist
INSERT INTO public.server_state (id, status, server_ip, launch_id, updated_by, updated_at)
VALUES (1, 'OFFLINE', '', 0, 'Initial Setup', NOW())
ON CONFLICT (id) DO NOTHING;

-- 3. Enable Row Level Security (RLS) for server_state
ALTER TABLE public.server_state ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "Allow anon select on server_state" ON public.server_state;
DROP POLICY IF EXISTS "Allow anon update on server_state" ON public.server_state;
DROP POLICY IF EXISTS "Allow anon insert on server_state" ON public.server_state;

-- 4. Create policies allowing anonymous reading and updating
CREATE POLICY "Allow anon select on server_state"
    ON public.server_state
    FOR SELECT
    TO anon
    USING (true);

CREATE POLICY "Allow anon update on server_state"
    ON public.server_state
    FOR UPDATE
    TO anon
    USING (true)
    WITH CHECK (true);

CREATE POLICY "Allow anon insert on server_state"
    ON public.server_state
    FOR INSERT
    TO anon
    WITH CHECK (true);

-- 5. Create user_tokens table for key-to-token lookups (e.g. "bablu" -> "Absfpypdx6XFjueps4TgJDjF")
CREATE TABLE IF NOT EXISTS public.user_tokens (
    key TEXT PRIMARY KEY,
    token TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- 6. Enable Row Level Security (RLS) for user_tokens
ALTER TABLE public.user_tokens ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "Allow anon select on user_tokens" ON public.user_tokens;
DROP POLICY IF EXISTS "Allow anon insert on user_tokens" ON public.user_tokens;
DROP POLICY IF EXISTS "Allow anon update on user_tokens" ON public.user_tokens;
DROP POLICY IF EXISTS "Allow anon insert/update on user_tokens" ON public.user_tokens;

CREATE POLICY "Allow anon select on user_tokens"
    ON public.user_tokens
    FOR SELECT
    TO anon
    USING (true);

CREATE POLICY "Allow anon insert on user_tokens"
    ON public.user_tokens
    FOR INSERT
    TO anon
    WITH CHECK (true);

CREATE POLICY "Allow anon update on user_tokens"
    ON public.user_tokens
    FOR UPDATE
    TO anon
    USING (true)
    WITH CHECK (true);
