static void u32_to_bytes(uint32_t u32, uint8_t *bytes)
{
	*bytes = (uint8_t) (u32 >> 24);
	*(bytes + 1) = (uint8_t) (u32 >> 16);
	*(bytes + 2) = (uint8_t) (u32 >> 8);
	*(bytes + 3) = (uint8_t) u32;
}

static void bytes_to_u32(uint8_t *bytes, uint32_t *u32)
{
	*u32 = (uint32_t) ((*(bytes) << 24) +
			   (*(bytes + 1) << 16) +
			   (*(bytes + 2) << 8) + (*(bytes + 3)));
}

static void u16_to_bytes(uint16_t u16, uint8_t *bytes)
{
	*bytes = (uint8_t) (u16 >> 8);
	*(bytes + 1) = (uint8_t) u16;
}


static void fat_entry_dir_deinit(void)
{
	struct rpmb_fat_entry *fe = NULL;

	if (!fat_entry_dir)
		return;

	if (!CFG_RPMB_FS_CACHE_ENTRIES) {
		fat_entry_dir_free();
		return;
	}

	fe = fat_entry_dir->rpmb_fat_entry_buf;
	fat_entry_dir->idx_curr = 0;
	fat_entry_dir->num_total_read = 0;
	fat_entry_dir->last_reached = false;

	if (fat_entry_dir->num_buffered > CFG_RPMB_FS_CACHE_ENTRIES) {
		fat_entry_dir->num_buffered = CFG_RPMB_FS_CACHE_ENTRIES;

		fe = realloc(fe, fat_entry_dir->num_buffered * sizeof(*fe));

		/*
		 * In case realloc fails, we are on the safe side if we destroy
		 * the whole structure. Upon the next init, the cache has to be
		 * re-established, but this case should not happen in practice.
		 */
		if (!fe)
			fat_entry_dir_free();
		else
			fat_entry_dir->rpmb_fat_entry_buf = fe;
	}
}


void tee_uuid_to_octets(uint8_t *d, const TEE_UUID *s)
{
	d[0] = s->timeLow >> 24;
	d[1] = s->timeLow >> 16;
	d[2] = s->timeLow >> 8;
	d[3] = s->timeLow;
	d[4] = s->timeMid >> 8;
	d[5] = s->timeMid;
	d[6] = s->timeHiAndVersion >> 8;
	d[7] = s->timeHiAndVersion;
	memcpy(d + 8, s->clockSeqAndNode, sizeof(s->clockSeqAndNode));
}

void tee_uuid_from_octets(TEE_UUID *d, const uint8_t *s)
{
	d->timeLow = SHIFT_U32(s[0], 24) | SHIFT_U32(s[1], 16) |
		     SHIFT_U32(s[2], 8) | s[3];
	d->timeMid = SHIFT_U32(s[4], 8) | s[5];
	d->timeHiAndVersion = SHIFT_U32(s[6], 8) | s[7];
	memcpy(d->clockSeqAndNode, s + 8, sizeof(d->clockSeqAndNode));
}
