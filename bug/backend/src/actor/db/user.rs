table! {
    use diesel::{types::Int4, sql_types::Text};
    use super::UserRoleMapping;

    users (id) {
        id -> Int4,
        uid -> Text,
        role -> UserRoleMapping,
    }
}

#[derive(Insertable, Queryable, Identifiable, Debug, PartialEq)]
#[table_name = "users"]
pub struct User {
    pub id: i32,
    pub uid: String,
    pub role: UserRole,
}

#[derive(DbEnum, Debug, PartialEq)]
pub enum UserRole {
    Admin,
}
