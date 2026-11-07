//! Minimal Modbus RTU master: build a "read holding registers" request, parse the reply.

pub fn crc16(data: &[u8]) -> u16 {
    let mut crc: u16 = 0xFFFF;
    for &b in data {
        crc ^= b as u16;
        for _ in 0..8 {
            crc = if crc & 1 != 0 { (crc >> 1) ^ 0xA001 } else { crc >> 1 };
        }
    }
    crc
}

pub fn read_holding_request(slave: u8, start: u16, count: u16) -> Vec<u8> {
    let mut f = vec![slave, 0x03, (start >> 8) as u8, start as u8, (count >> 8) as u8, count as u8];
    let crc = crc16(&f);
    f.push(crc as u8);
    f.push((crc >> 8) as u8);
    f
}

#[derive(Debug, PartialEq)]
pub enum ParseError {
    TooShort,
    BadCrc,
    Exception(u8),
    Unexpected,
}

pub fn parse_read_holding_reply(slave: u8, frame: &[u8]) -> Result<Vec<u16>, ParseError> {
    if frame.len() < 5 {
        return Err(ParseError::TooShort);
    }
    let (body, crc_bytes) = frame.split_at(frame.len() - 2);
    if crc16(body) != u16::from_le_bytes([crc_bytes[0], crc_bytes[1]]) {
        return Err(ParseError::BadCrc);
    }
    if body[0] != slave {
        return Err(ParseError::Unexpected);
    }
    if body[1] == 0x83 {
        return Err(ParseError::Exception(body[2]));
    }
    if body[1] != 0x03 || body.len() != 3 + body[2] as usize {
        return Err(ParseError::Unexpected);
    }
    Ok(body[3..].chunks(2).map(|c| u16::from_be_bytes([c[0], c[1]])).collect())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn crc_matches_spec_example() {
        assert_eq!(crc16(&[0x01, 0x03, 0x00, 0x00, 0x00, 0x01]), 0x0A84);
    }

    #[test]
    fn request_has_valid_crc() {
        let r = read_holding_request(1, 0, 3);
        assert_eq!(r.len(), 8);
        assert_eq!(crc16(&r[..6]).to_le_bytes(), [r[6], r[7]]);
    }

    #[test]
    fn parses_good_reply() {
        let mut f = vec![1, 3, 6, 0, 215, 0, 42, 0, 7];
        let c = crc16(&f);
        f.extend_from_slice(&c.to_le_bytes());
        assert_eq!(parse_read_holding_reply(1, &f), Ok(vec![215, 42, 7]));
    }

    #[test]
    fn rejects_bad_crc_and_exception() {
        let mut f = vec![1, 3, 2, 0, 5];
        let c = crc16(&f);
        f.extend_from_slice(&c.to_le_bytes());
        let last = f.len() - 1;
        f[last] ^= 0xFF;
        assert_eq!(parse_read_holding_reply(1, &f), Err(ParseError::BadCrc));

        let mut e = vec![1, 0x83, 2];
        let c = crc16(&e);
        e.extend_from_slice(&c.to_le_bytes());
        assert_eq!(parse_read_holding_reply(1, &e), Err(ParseError::Exception(2)));
    }
}
